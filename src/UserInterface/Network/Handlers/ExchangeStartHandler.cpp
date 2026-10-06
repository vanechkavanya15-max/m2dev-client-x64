#include "../../StdAfx.h"
/**
 * @file ExchangeStartHandler.cpp
 * @brief Nowoczesny handler C++23 dla pakietu rozpoczecia wymiany.
 */

#include "../../Packet.h"
#include "../../PythonExchange.h"
#include "../../PythonCharacterManager.h"
#include "../../PythonPlayer.h"
#include "../../Core/EventBus.h"
#include "../../../EterBase/Result.h"
#include "../../../EterBase/StrongTypes.h"
#include "../../../EterBase/LogModern.h"
#include "ExchangeStartHandler.h"

#include <span>
#include <string>
#include <optional>

namespace Network::Handlers {

/**
 * @brief Przetwarza pakiet sieciowy inicjujacy sesje handlu (wymiane) miedzy graczami.
 * 
 * Funkcja weryfikuje dlugosc bufora, bezpiecznie aktualizuje stan `CPythonExchange`
 * w pamieci C++ oraz publikuje zdarzenie `ExchangeStartEvent` poprzez `EventBus`,
 * eliminujac bezposrednie wywolania do warstwy UI w Pythonie.
 * 
 * @param buffer Zrzut pamieci pakietu sieciowego (span na bajty).
 * @return EterBase::PacketResult<void> Sukces operacji lub odpowiedni kod bledu.
 */
EterBase::PacketResult<void> HandleExchangeStart(std::span<const uint8_t> buffer) {
    if (buffer.size() < sizeof(TPacketGCExchange)) {
        EterBase::ModernLogger::Error("ExchangeStartHandler: Otrzymano zbyt krotki bufor pakietu ({} < {}).", 
            buffer.size(), sizeof(TPacketGCExchange));
        return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
    }

    const auto* packet = reinterpret_cast<const TPacketGCExchange*>(buffer.data());

    // Obowiazkowe uzycie silnych typow
    EterBase::EntityId targetId{packet->arg1};

    // Aktualizacja stanu wymiany w pamieci
    auto& exchange = CPythonExchange::Instance();
    exchange.Clear();
    exchange.Start();
    exchange.SetSelfName(CPythonPlayer::Instance().GetName());

    // Bezpieczne wyluskanie celu przy uzyciu std::optional (C++23)
    auto* instancePtr = CPythonCharacterManager::Instance().GetInstancePtr(targetId.value());
    std::optional<CInstanceBase*> targetInstance = 
        instancePtr ? std::make_optional(instancePtr) : std::nullopt;

    std::string targetNameStr = targetInstance
        .transform([](CInstanceBase* instance) { 
            return std::string(instance->GetNameString()); 
        })
        .value_or("Unknown");

    exchange.SetTargetName(targetNameStr.c_str());

    EterBase::ModernLogger::Info("ExchangeStartHandler: Rozpoczecie wymiany z {}. EntityId: {}.", 
        targetNameStr, targetId.value());

    // Publikacja eventu aby odpiac handler od bezposrednich zmian w GUI (Core::EventBus)
    UserInterface::Core::EventBus::GetInstance().Publish(ExchangeStartEvent{targetId, targetNameStr});

    return {};
}

} // namespace Network::Handlers
