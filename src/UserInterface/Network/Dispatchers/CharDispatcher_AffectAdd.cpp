#include "../../StdAfx.h"
#include "../../Packet.h"
#include "../../PythonNetworkStream.h"
#include "../../PythonPlayer.h"
#include "../../PythonApplication.h"
#include "../../Core/EventBus.h"
#include "../../Core/Events/AffectEvents.h"
#include "src/EterBase/Result.h"
#include "src/EterBase/StrongTypes.h"
#include "src/EterBase/Logger.h"

namespace UserInterface::Network::Dispatchers {

/**
 * @brief Dispatcher for TPacketGCAffectAdd packet.
 */
class CharDispatcher_AffectAdd {
public:
    /**
     * @brief Processes the TPacketGCAffectAdd packet.
     * @param stream The network stream to read the packet from.
     * @return EterBase::PacketResult<void> indicating success or failure.
     */
    static EterBase::PacketResult<void> Process(CPythonNetworkStream* stream) {
        if (!stream) {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "CharDispatcher_AffectAdd: Stream is null.");
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        TPacketGCAffectAdd packet{};
        if (!stream->Recv(sizeof(packet), &packet)) {
            EterBase::ModernLogger::Log(EterBase::LogLevel::Error, "CharDispatcher_AffectAdd: Failed to receive TPacketGCAffectAdd.");
            return std::unexpected(EterBase::PacketError::BufferUnderflow);
        }

        const auto& element = packet.elem;

        EterBase::ModernLogger::Log(EterBase::LogLevel::Debug, "CharDispatcher_AffectAdd: Received affect type: {}, pointIdx: {}, value: {}, duration: {}, flag: {}",
            element.dwType, element.bPointIdxApplyOn, element.lApplyValue, element.lDuration, element.dwFlag);

        if (element.bPointIdxApplyOn == POINT_ENERGY) {
            time_t server_time = CPythonApplication::Instance().GetServerTimeStamp();
            CPythonPlayer::Instance().SetStatus(POINT_ENERGY_END_TIME, server_time + element.lDuration);
        }

        UserInterface::Core::Events::AffectAddEvent event{
            element.dwType,
            element.bPointIdxApplyOn,
            element.lApplyValue,
            element.dwFlag,
            element.lDuration
        };

        UserInterface::Core::EventBus::GetInstance().Publish(event);

        return {};
    }
};

} // namespace UserInterface::Network::Dispatchers
