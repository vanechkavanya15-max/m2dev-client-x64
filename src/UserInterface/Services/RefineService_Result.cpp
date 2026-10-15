//#include "../StdAfx.h"
#include "IPlayerStatsService.h"
#include "../Packet.h"
#include "../Core/EventBus.h"
#include "EterBase/Result.h"
#include "EterBase/LogModern.h"

#include <span>
#include <cstdint>
#include <cstring>
#include <expected>

/**
 * @file RefineService_Result.cpp
 * @brief Implementacja obslugi rezultatow ulepszania przedmiotow w oparciu o C++23.
 * 
 * Modul analizuje sieciowe pakiety rezultatow ulepszania (w tym wypadku na bazie struktury Dragon Soul, 
 * ktora dostarcza statusow REFINE_SUCCEED / REFINE_FAIL), zglasza bledy walidacji uzywajac typow 
 * EterBase::PacketResult<void> i publikuje zdarzenia domenowe dla UI za posrednictwem szyny zdarzen EventBus,
 * realizujac zasade Single Responsibility Principle.
 */

namespace UserInterface::Core::Events
{
    /**
     * @brief Zdarzenie rozglaszane po zakonczeniu procesu ulepszania przedmiotu.
     */
    struct RefineResultEvent : public IEvent
    {
        uint8_t subType;
        uint8_t windowType;
        uint16_t cell;

        RefineResultEvent(uint8_t subType, uint8_t windowType, uint16_t cell)
            : subType(subType), windowType(windowType), cell(cell) {}
    };
}

namespace UserInterface::Services
{
    /**
     * @brief Handler analizujacy pakiety z wynikiem ulepszenia przedmiotu.
     */
    class RefineResultHandler
    {
    public:
        RefineResultHandler() = default;
        ~RefineResultHandler() = default;

        // Blokada kopiowania i przenoszenia (stateless handler)
        RefineResultHandler(const RefineResultHandler&) = delete;
        RefineResultHandler& operator=(const RefineResultHandler&) = delete;

        /**
         * @brief Przetwarza pakiet wyniku ulepszania.
         * 
         * @param payload Bufor z odebranymi danymi pakietu.
         * @param statsService Zaleznosc do serwisu statystyk (wstrzykiwana dla potencjalnych operacji, np. modyfikacja kosztow).
         * @return EterBase::PacketResult<void> Sukces (void) lub PacketError w przypadku bledu odczytu.
         */
        EterBase::PacketResult<void> HandleRefineResultPacket(std::span<const uint8_t> payload, IPlayerStatsService& statsService) const
        {
            if (payload.size() < sizeof(TPacketGCDragonSoulRefine))
            {
                EterBase::ModernLogger::Error(
                    "RefineResultHandler: Zbyt krotki pakiet (oczekiwano: {}, otrzymano: {})", 
                    sizeof(TPacketGCDragonSoulRefine), payload.size()
                );
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            TPacketGCDragonSoulRefine packet;
            std::memcpy(&packet, payload.data(), sizeof(TPacketGCDragonSoulRefine));

            if (packet.header != GC::DRAGON_SOUL_REFINE)
            {
                EterBase::ModernLogger::Error(
                    "RefineResultHandler: Nieprawidlowy naglowek pakietu (oczekiwano: {}, otrzymano: {})", 
                    static_cast<int>(GC::DRAGON_SOUL_REFINE), static_cast<int>(packet.header)
                );
                return EterBase::MakeError(EterBase::PacketError::InvalidHeader);
            }

            bool isHandledResult = false;
            switch (packet.bSubType)
            {
                case DragonSoulSub::REFINE_SUCCEED:
                    EterBase::ModernLogger::Info(
                        "RefineResultHandler: Sukces ulepszenia przedmiotu (Window: {}, Cell: {})", 
                        static_cast<int>(packet.Pos.window_type), static_cast<int>(packet.Pos.cell)
                    );
                    isHandledResult = true;
                    break;
                case DragonSoulSub::REFINE_FAIL:
                case DragonSoulSub::REFINE_FAIL_MAX_REFINE:
                case DragonSoulSub::REFINE_FAIL_INVALID_MATERIAL:
                case DragonSoulSub::REFINE_FAIL_NOT_ENOUGH_MONEY:
                case DragonSoulSub::REFINE_FAIL_NOT_ENOUGH_MATERIAL:
                case DragonSoulSub::REFINE_FAIL_TOO_MUCH_MATERIAL:
                    EterBase::ModernLogger::Info(
                        "RefineResultHandler: Porazka ulepszenia przedmiotu (Typ Bledu: {}, Window: {}, Cell: {})", 
                        static_cast<int>(packet.bSubType), static_cast<int>(packet.Pos.window_type), static_cast<int>(packet.Pos.cell)
                    );
                    isHandledResult = true;
                    break;
                default:
                    // Inne podtypy pakietu ulepszania zignorujemy, interesuje nas tylko SUKCES/PORAZKA.
                    break;
            }

            if (isHandledResult)
            {
                // Powiadomienie warstwy UI
                Core::EventBus::Instance().Publish(
                    Core::Events::RefineResultEvent(packet.bSubType, packet.Pos.window_type, packet.Pos.cell)
                );
            }

            return {};
        }
    };
}
