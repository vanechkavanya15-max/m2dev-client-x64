#include "BotApi_FFI.h"

#include "Client/Contracts/IPlayerDomainContract.h"
#include "Client/Contracts/IWorldQueryContract.h"
#include "Client/Contracts/ICombatDomainContract.h"
#include "Client/Contracts/IInventoryDomainContract.h"
#include "Client/MCP/AgentMCPBridge.h"

#include <atomic>
#include <cmath>
#include <vector>
#include <algorithm>
#include <mutex>

// Walidacja binarna rozmiarow struktur C-ABI
static_assert(sizeof(ActorSnapshot) == 32, "ActorSnapshot rozmiar musi wynosic dokladnie 32 bajty (wyrownanie do 4/8 bajtow)");
static_assert(sizeof(GroundItemSnapshot) == 20, "GroundItemSnapshot rozmiar musi wynosic dokladnie 20 bajtow");

namespace
{
    // Wskazniki atomowe na wstrzykniete kontrakty domenowe (ochrona przed wyscigami danych)
    std::atomic<Client::Contracts::IPlayerDomainContract*> s_pPlayerContract{nullptr};
    std::atomic<Client::Contracts::IWorldQueryContract*> s_pWorldContract{nullptr};
    std::atomic<Client::Contracts::ICombatDomainContract*> s_pCombatContract{nullptr};
    std::atomic<Client::Contracts::IInventoryDomainContract*> s_pInventoryContract{nullptr};

    // Mockowane przedmioty na ziemi z blokada mutex
    std::vector<GroundItemSnapshot> s_mockGroundItems;
    std::mutex s_mockItemsMutex;

    // Flagi stanu okna i dlawienia renderowania
    std::atomic<bool> s_windowVisible{true};
    std::atomic<bool> s_renderThrottle{false};

    // Uchwyty wywolan zwrotnych okna (atomowe ze wzgledu na zewnetrzny dostep)
    std::atomic<BotWindowVisibilityFn> s_pWindowVisibilityFn{nullptr};
    std::atomic<BotWindowVisibilityQueryFn> s_pWindowVisibilityQueryFn{nullptr};
    std::atomic<BotRenderThrottleFn> s_pRenderThrottleFn{nullptr};

    inline Client::Contracts::IPlayerDomainContract* GetPlayer() noexcept
    {
        auto* p = s_pPlayerContract.load(std::memory_order_acquire);
        if (p)
            return p;
        return Client::MCP::AgentMCPBridge::Instance().GetPlayerContract();
    }

    inline Client::Contracts::IWorldQueryContract* GetWorld() noexcept
    {
        auto* p = s_pWorldContract.load(std::memory_order_acquire);
        if (p)
            return p;
        return Client::MCP::AgentMCPBridge::Instance().GetWorldContract();
    }

    inline Client::Contracts::ICombatDomainContract* GetCombat() noexcept
    {
        auto* p = s_pCombatContract.load(std::memory_order_acquire);
        if (p)
            return p;
        return Client::MCP::AgentMCPBridge::Instance().GetCombatContract();
    }

    inline Client::Contracts::IInventoryDomainContract* GetInventory() noexcept
    {
        auto* p = s_pInventoryContract.load(std::memory_order_acquire);
        if (p)
            return p;
        return Client::MCP::AgentMCPBridge::Instance().GetInventoryContract();
    }
}

// ============================================================================
// IMPLEMENTACJA EKSPORTOWANYCH SYGNATUR C-ABI
// ============================================================================

bool Bot_GetPlayerPos(float* outX, float* outY, float* outZ)
{
    try
    {
        if (outX) *outX = 0.0f;
        if (outY) *outY = 0.0f;
        if (outZ) *outZ = 0.0f;

        auto* pPlayer = GetPlayer();
        if (!pPlayer)
            return false;

        auto coords = pPlayer->GetCoordinates();
        if (outX) *outX = coords.x;
        if (outY) *outY = coords.y;
        if (outZ) *outZ = coords.z;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

int64_t Bot_GetPlayerStatus(int statusType)
{
    try
    {
        auto* pPlayer = GetPlayer();
        if (!pPlayer)
            return 0;

        return pPlayer->GetStatus(statusType);
    }
    catch (...)
    {
        return 0;
    }
}

int Bot_GetActorsInRadius(float radius, ActorSnapshot* outBuf, int maxCapacity)
{
    try
    {
        if (!outBuf || maxCapacity <= 0 || !std::isfinite(radius) || radius <= 0.0f)
            return 0;

        auto* pWorld = GetWorld();
        if (!pWorld)
            return 0;

        struct Context {
            ActorSnapshot* buf;
            int maxCap;
            int count;
        } ctx{outBuf, maxCapacity, 0};

        // Bezalokacyjne przejscie przez encje swiata
        pWorld->ForEachEntityInRadius(radius, [](const Client::Contracts::EntitySnapshot& e, void* userData) {
            auto* c = static_cast<Context*>(userData);
            if (c->count >= c->maxCap)
                return;

            // Filtrujemy nie-aktorow (przedmioty na ziemi, drzwi, portale, encje nieznane)
            if (e.type == Client::Contracts::EntityType::ItemGround ||
                e.type == Client::Contracts::EntityType::Unknown ||
                e.type == Client::Contracts::EntityType::Door ||
                e.type == Client::Contracts::EntityType::Warp)
                return;

            ActorSnapshot& snap = c->buf[c->count];
            snap.vid = e.vid;
            snap.vnum = e.vnum;
            snap.x = e.x;
            snap.y = e.y;
            snap.z = e.z;
            snap.hp = static_cast<int32_t>(e.currentHpPercent);
            snap.max_hp = 100;
            snap.is_dead = e.isDead ? 1 : 0;
            snap._reserved = 0;

            switch (e.type)
            {
                case Client::Contracts::EntityType::Player:  snap.type = 6; break;
                case Client::Contracts::EntityType::Stone:   snap.type = 2; break;
                case Client::Contracts::EntityType::NPC:     snap.type = 1; break;
                case Client::Contracts::EntityType::Monster: snap.type = 0; break;
                default:                                     snap.type = 0; break;
            }

            ++c->count;
        }, &ctx);

        return ctx.count;
    }
    catch (...)
    {
        return 0;
    }
}

int Bot_GetGroundItems(float radius, GroundItemSnapshot* outBuf, int maxCapacity)
{
    try
    {
        if (!outBuf || maxCapacity <= 0 || !std::isfinite(radius) || radius <= 0.0f)
            return 0;

        int count = 0;

        // 1. Sprawdzenie mockowanych przedmiotow (zabezpieczone mutexem)
        {
            std::lock_guard<std::mutex> lock(s_mockItemsMutex);
            if (!s_mockGroundItems.empty())
            {
                float pX = 0.0f, pY = 0.0f, pZ = 0.0f;
                Bot_GetPlayerPos(&pX, &pY, &pZ);
                float radSq = radius * radius;

                for (const auto& item : s_mockGroundItems)
                {
                    if (count >= maxCapacity)
                        break;

                    float dx = item.x - pX;
                    float dy = item.y - pY;
                    if (dx * dx + dy * dy <= radSq)
                    {
                        outBuf[count++] = item;
                    }
                }
                return count;
            }
        }

        // 2. Pobranie z kontraktu swiata
        auto* pWorld = GetWorld();
        if (pWorld)
        {
            struct Context {
                GroundItemSnapshot* buf;
                int maxCap;
                int count;
            } ctx{outBuf, maxCapacity, 0};

            pWorld->ForEachEntityInRadius(radius, [](const Client::Contracts::EntitySnapshot& e, void* userData) {
                auto* c = static_cast<Context*>(userData);
                if (c->count >= c->maxCap)
                    return;

                if (e.type == Client::Contracts::EntityType::ItemGround)
                {
                    GroundItemSnapshot& snap = c->buf[c->count];
                    snap.vid = e.vid;
                    snap.vnum = e.vnum;
                    snap.x = e.x;
                    snap.y = e.y;
                    snap.count = 1;
                    ++c->count;
                }
            }, &ctx);

            return ctx.count;
        }

        return 0;
    }
    catch (...)
    {
        return 0;
    }
}

bool Bot_MoveTo(float x, float y)
{
    try
    {
        if (!std::isfinite(x) || !std::isfinite(y))
            return false;

        auto* pPlayer = GetPlayer();
        if (pPlayer)
        {
            return pPlayer->RequestMove(x, y);
        }
        return false; // KRYTYCZNA POPRAWKA: false gdy brak aktywnego gracza
    }
    catch (...)
    {
        return false;
    }
}

bool Bot_AttackTarget(uint32_t vid)
{
    try
    {
        if (vid == 0)
            return false;

        auto* pCombat = GetCombat();
        if (pCombat)
        {
            auto verdict = pCombat->EvaluateAttack(vid);
            if (!verdict.canAttack)
                return false;
            return pCombat->ExecuteAttack(vid);
        }
        return false;
    }
    catch (...)
    {
        return false;
    }
}

bool Bot_UseItem(uint8_t windowType, uint16_t cell)
{
    try
    {
        auto* pInventory = GetInventory();
        if (pInventory)
        {
            Client::Contracts::ItemCellAddress addr{windowType, cell};
            return pInventory->RequestUseItem(addr);
        }
        return false;
    }
    catch (...)
    {
        return false;
    }
}

void Bot_SetWindowVisibility(bool visible)
{
    try
    {
        s_windowVisible.store(visible, std::memory_order_release);
        auto fn = s_pWindowVisibilityFn.load(std::memory_order_acquire);
        if (fn)
        {
            fn(visible);
        }
    }
    catch (...) {}
}

bool Bot_IsWindowVisible()
{
    try
    {
        auto qFn = s_pWindowVisibilityQueryFn.load(std::memory_order_acquire);
        if (qFn)
        {
            return qFn();
        }
        return s_windowVisible.load(std::memory_order_acquire);
    }
    catch (...)
    {
        return false;
    }
}

void Bot_SetRenderThrottle(bool throttle)
{
    try
    {
        s_renderThrottle.store(throttle, std::memory_order_release);
        auto fn = s_pRenderThrottleFn.load(std::memory_order_acquire);
        if (fn)
        {
            fn(throttle);
        }
    }
    catch (...) {}
}

bool Bot_IsRenderThrottled()
{
    return s_renderThrottle.load(std::memory_order_acquire);
}

// ============================================================================
// FUNKCJE POMOCNICZE DLA KONTRAKTOW DOMENOWYCH I TESTOW
// ============================================================================

void Bot_InitializeContracts(
    Client::Contracts::IPlayerDomainContract* pPlayer,
    Client::Contracts::IWorldQueryContract* pWorld,
    Client::Contracts::ICombatDomainContract* pCombat,
    Client::Contracts::IInventoryDomainContract* pInventory
)
{
    s_pPlayerContract.store(pPlayer, std::memory_order_release);
    s_pWorldContract.store(pWorld, std::memory_order_release);
    s_pCombatContract.store(pCombat, std::memory_order_release);
    s_pInventoryContract.store(pInventory, std::memory_order_release);
}

void Bot_ResetContracts()
{
    s_pPlayerContract.store(nullptr, std::memory_order_release);
    s_pWorldContract.store(nullptr, std::memory_order_release);
    s_pCombatContract.store(nullptr, std::memory_order_release);
    s_pInventoryContract.store(nullptr, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(s_mockItemsMutex);
        s_mockGroundItems.clear();
    }
    s_windowVisible.store(true, std::memory_order_release);
    s_renderThrottle.store(false, std::memory_order_release);
    s_pWindowVisibilityFn.store(nullptr, std::memory_order_release);
    s_pWindowVisibilityQueryFn.store(nullptr, std::memory_order_release);
    s_pRenderThrottleFn.store(nullptr, std::memory_order_release);
}

void Bot_SetMockGroundItems(const GroundItemSnapshot* items, int count) noexcept
{
    try
    {
        std::lock_guard<std::mutex> lock(s_mockItemsMutex);
        s_mockGroundItems.clear();
        if (items && count > 0)
        {
            s_mockGroundItems.assign(items, items + count);
        }
    }
    catch (...)
    {
    }
}

void Bot_SetWindowControlHandlers(
    BotWindowVisibilityFn onSetVisible,
    BotWindowVisibilityQueryFn onQueryVisible,
    BotRenderThrottleFn onSetThrottle
)
{
    s_pWindowVisibilityFn.store(onSetVisible, std::memory_order_release);
    s_pWindowVisibilityQueryFn.store(onQueryVisible, std::memory_order_release);
    s_pRenderThrottleFn.store(onSetThrottle, std::memory_order_release);
}
