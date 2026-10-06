#include "../../StdAfx.h"
#include "PartyUpdateHandler.h"
#include "../../PythonPlayer.h"
#include "../../../EterBase/LogModern.h"
#include <optional>
#include <ranges>
#include <algorithm>

namespace Network::Handlers
{
    EterBase::PacketResult<void> ProcessPartyUpdate(std::span<const uint8_t> buffer)
    {
        if (buffer.size() < sizeof(PacketPartyUpdate))
        {
            EterBase::ModernLogger::Error("ProcessPartyUpdate: Buffer size {} is smaller than sizeof(PacketPartyUpdate) {}", buffer.size(), sizeof(PacketPartyUpdate));
            return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
        }

        const auto* packet = reinterpret_cast<const PacketPartyUpdate*>(buffer.data());
        const EterBase::EntityId pid(packet->pid);

        auto getMemberOpt = [](uint32_t memberPid) -> std::optional<CPythonPlayer::TPartyMemberInfo*> {
            CPythonPlayer::TPartyMemberInfo* member = nullptr;
            if (CPythonPlayer::Instance().GetPartyMemberPtr(memberPid, &member))
            {
                return member;
            }
            return std::nullopt;
        };

        bool found = getMemberOpt(pid.value())
            .transform([packet, pid](CPythonPlayer::TPartyMemberInfo* member) {
                // Not używamy member w tym przykładzie bo API CPythonPlayer wymaga wywołań w taki sposób,
                // ale moglibyśmy ewentualnie użyć member gdybyśmy go modyfikowali - pozostawione dla kompatybilności API.
                CPythonPlayer::Instance().UpdatePartyMemberInfo(pid.value(), packet->state, packet->percent_hp);
                
                constexpr size_t numAffects = std::size(packet->affects);
                for (size_t i = 0; i < numAffects; ++i)
                {
                    CPythonPlayer::Instance().UpdatePartyMemberAffect(pid.value(), static_cast<uint8_t>(i), packet->affects[i]);
                }
                return true;
            })
            .value_or(false);

        if (!found)
        {
            EterBase::ModernLogger::Warn("ProcessPartyUpdate: Party member {} not found", pid.value());
            return {};
        }

        // Emitowanie zdarzenia (odpiecie od GUI)
        UserInterface::Core::EventBus::GetInstance().Publish(PartyUpdateEvent{ pid.value() });

        return {};
    }

    bool HandlePartyUpdate(std::span<const uint8_t> buffer)
    {
        return ProcessPartyUpdate(buffer).has_value();
    }
}
