#include "../StdAfx.h"
#include "IPlayerStatsService.h"
#include "../Packet.h"
#include "EterBase/Result.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "../Core/EventBus.h"

#include <vector>
#include <memory>
#include <optional>
#include <span>

namespace UserInterface::Services
{
    /**
     * @brief Zdarzenie rozglaszane po pomyslnym otwarciu i zaktualizowaniu dialogu ulepszania.
     */
    struct RefineWindowOpenedEvent : public Core::IEvent
    {
        uint8_t type;
        EterBase::ItemSlot pos;
        TRefineTable refineTable;

        RefineWindowOpenedEvent(uint8_t t, EterBase::ItemSlot p, const TRefineTable& table)
            : type(t), pos(p), refineTable(table) {}
    };

    /**
     * @brief Zdarzenie zamkniecia okna ulepszania.
     */
    struct RefineWindowClosedEvent : public Core::IEvent
    {
    };

    /**
     * @brief Mikro-serwis zarzadzajacy stanem dialogu ulepszania u kowala.
     * Odpowiada za weryfikacje wymaganych ulepszaczy oraz yang na podstawie IPlayerStatsService.
     */
    class RefineService_Window
    {
    public:
        explicit RefineService_Window(std::shared_ptr<IPlayerStatsService> statsService)
            : m_statsService(std::move(statsService)), m_isOpen(false)
        {
        }

        /**
         * @brief Przetwarza pakiet informacyjny o ulepszaniu i aktualizuje stan wewnetrzny.
         * @param packetSpan Span zawierajacy dane pakietu ulepszania.
         * @return Wynik operacji sieciowej.
         */
        EterBase::PacketResult<void> HandleRefineInformation(std::span<const uint8_t> packetSpan)
        {
            if (packetSpan.size() < sizeof(TPacketGCRefineInformationNew))
            {
                EterBase::ModernLogger::Error("RefineService_Window: Packet too small.");
                return EterBase::MakeError(EterBase::PacketError::BufferUnderflow);
            }

            const auto* packet = reinterpret_cast<const TPacketGCRefineInformationNew*>(packetSpan.data());

            m_isOpen = true;
            m_currentType = packet->type;
            m_currentPos = EterBase::ItemSlot(packet->pos);
            m_currentTable = packet->refine_table;

            EterBase::ModernLogger::Info("RefineService_Window: Refine window opened. Type: {}, Pos: {}, Cost: {}, Prob: {}",
                m_currentType, m_currentPos.value(), m_currentTable->cost, m_currentTable->prob);

            Core::EventBus::GetInstance().Publish(RefineWindowOpenedEvent(
                m_currentType, m_currentPos, *m_currentTable
            ));

            return {};
        }

        /**
         * @brief Zamyka okno ulepszania i czysci stan.
         * @return Wynik operacji.
         */
        EterBase::PacketResult<void> CloseRefineWindow()
        {
            if (!m_isOpen)
            {
                EterBase::ModernLogger::Warning("RefineService_Window: Tried to close refine window but it was not open.");
                return EterBase::MakeError(EterBase::PacketError::SessionClosed);
            }

            m_isOpen = false;
            m_currentTable = std::nullopt;

            EterBase::ModernLogger::Info("RefineService_Window: Refine window closed.");
            Core::EventBus::GetInstance().Publish(RefineWindowClosedEvent());

            return {};
        }

        /**
         * @brief Sprawdza czy gracz posiada wystarczajaca ilosc yang na ulepszenie.
         * @return Oczekiwana wartosc boolean lub blad domenowy jesli okno jest zamkniete.
         */
        std::expected<bool, EterBase::EntityError> CanAffordRefine() const
        {
            if (!m_isOpen || !m_currentTable.has_value())
            {
                return EterBase::MakeError(EterBase::EntityError::NotFound);
            }

            if (!m_statsService)
            {
                EterBase::ModernLogger::Error("RefineService_Window: IPlayerStatsService is null.");
                return EterBase::MakeError(EterBase::EntityError::NotFound);
            }

            const int64_t currentGold = m_statsService->GetPoints().gold;
            const bool canAfford = currentGold >= m_currentTable->cost;

            EterBase::ModernLogger::Debug("RefineService_Window: CanAffordRefine: gold={}, cost={}, canAfford={}",
                currentGold, m_currentTable->cost, canAfford);

            return canAfford;
        }

        /**
         * @brief Zwraca identyfikator przedmiotu docelowego.
         */
        std::expected<EterBase::ItemVnum, EterBase::EntityError> GetResultItemVnum() const
        {
            if (!m_isOpen || !m_currentTable.has_value())
            {
                return EterBase::MakeError(EterBase::EntityError::NotFound);
            }

            return EterBase::ItemVnum(m_currentTable->result_vnum);
        }

        /**
         * @brief Zwraca identyfikator przedmiotu zrodlowego.
         */
        std::expected<EterBase::ItemVnum, EterBase::EntityError> GetSourceItemVnum() const
        {
            if (!m_isOpen || !m_currentTable.has_value())
            {
                return EterBase::MakeError(EterBase::EntityError::NotFound);
            }

            return EterBase::ItemVnum(m_currentTable->src_vnum);
        }

    private:
        std::shared_ptr<IPlayerStatsService> m_statsService;
        bool m_isOpen;
        uint8_t m_currentType{0};
        EterBase::ItemSlot m_currentPos{EterBase::ItemSlot{0}};
        std::optional<TRefineTable> m_currentTable;
    };
}
