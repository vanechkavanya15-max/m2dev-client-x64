#pragma once

#include <cstdint>
#include <cstddef>

#ifndef BOT_API
    #ifdef __cplusplus
        #if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
            #define BOT_API extern "C" __declspec(dllexport)
        #else
            #define BOT_API extern "C" __attribute__((visibility("default")))
        #endif
    #else
        #if defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
            #define BOT_API __declspec(dllexport)
        #else
            #define BOT_API __attribute__((visibility("default")))
        #endif
    #endif
#endif

// ============================================================================
// STRUKTURY DANYCH C-ABI (ZERO-SOL2, ZERO-GC, PRAGMA PACK 1)
// ============================================================================
#pragma pack(push, 1)
struct ActorSnapshot {
    uint32_t vid;       // 0..3 (4B)
    uint32_t vnum;      // 4..7 (4B)
    float x, y, z;      // 8..19 (12B)
    int32_t hp, max_hp; // 20..27 (8B)
    uint8_t type;       // 28 (1B): 0=mob, 1=npc, 2=metin, 6=player
    uint8_t is_dead;    // 29 (1B): 0=zyje, 1=martwy
    uint16_t _reserved; // 30..31 (2B): wyrownanie do 32 bajtow (4-bajtowa granica pamieci)
};

struct GroundItemSnapshot {
    uint32_t vid, vnum; // 0..7 (8B)
    float x, y;         // 8..15 (8B)
    uint32_t count;     // 16..19 (4B)
};
#pragma pack(pop)

// ============================================================================
// EKSPORTOWANE SYGNATURY C-ABI DLA AUTOMATYZACJI I BOTOW
// ============================================================================
BOT_API bool Bot_GetPlayerPos(float* outX, float* outY, float* outZ);
BOT_API int64_t Bot_GetPlayerStatus(int statusType);
BOT_API int Bot_GetActorsInRadius(float radius, ActorSnapshot* outBuf, int maxCapacity);
BOT_API int Bot_GetGroundItems(float radius, GroundItemSnapshot* outBuf, int maxCapacity);
BOT_API bool Bot_MoveTo(float x, float y);
BOT_API bool Bot_AttackTarget(uint32_t vid);
BOT_API bool Bot_UseItem(uint8_t windowType, uint16_t cell);
BOT_API void Bot_SetWindowVisibility(bool visible);
BOT_API void Bot_SetRenderThrottle(bool throttle);
BOT_API bool Bot_IsWindowVisible();

// ============================================================================
// FUNKCJE POMOCNICZE I INTEGRACYJNE DLA KONTRAKTOW DOMENOWYCH I TESTOW
// ============================================================================
#ifdef __cplusplus
namespace Client::Contracts
{
    class IPlayerDomainContract;
    class IWorldQueryContract;
    class ICombatDomainContract;
    class IInventoryDomainContract;
}

BOT_API void Bot_InitializeContracts(
    Client::Contracts::IPlayerDomainContract* pPlayer,
    Client::Contracts::IWorldQueryContract* pWorld,
    Client::Contracts::ICombatDomainContract* pCombat,
    Client::Contracts::IInventoryDomainContract* pInventory
);

BOT_API void Bot_ResetContracts();
BOT_API bool Bot_IsRenderThrottled();
BOT_API void Bot_SetMockGroundItems(const GroundItemSnapshot* items, int count) noexcept;

typedef void (*BotWindowVisibilityFn)(bool visible);
typedef bool (*BotWindowVisibilityQueryFn)();
typedef void (*BotRenderThrottleFn)(bool throttle);

BOT_API void Bot_SetWindowControlHandlers(
    BotWindowVisibilityFn onSetVisible,
    BotWindowVisibilityQueryFn onQueryVisible,
    BotRenderThrottleFn onSetThrottle
);
#endif
