#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <expected>
#include <string_view>
#include <format>

#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"
#include "../Core/EventBus.h"

/**
 * @file DungeonStateModel.h
 * @brief Model stanu lochu (Dungeon) dla architektury C++23, odpiety od GUI.
 *
 * Klasa utrzymuje stan instancji lochu klienta, emitujac zdarzenia
 * z uzyciem wzorca EventBus, umozliwiajac innym systemom reakcje na zmiany
 * w bezpieczny, synchroniczny lub asynchroniczny sposob bez notacji wegierskiej
 * i starych parametrow wyjsciowych.
 */

namespace UserInterface::Domain {

/**
 * @brief Zdarzenie aktualizacji stanu lochu publikowane przez EventBus.
 */
struct DungeonStateUpdatedEvent : public Core::IEvent {
    EterBase::DungeonId id;
    std::optional<uint32_t> currentFloor;
    std::optional<uint32_t> remainingTime;
    std::optional<uint32_t> killedMonsters;
    std::optional<uint32_t> destroyedMetins;

    /**
     * @brief Konstruktor inicjalizujacy pelny stan po aktualizacji.
     * @param id Identyfikator instancji lochu.
     * @param currentFloor Opcjonalny aktualny poziom lochu.
     * @param remainingTime Opcjonalny pozostaly czas do zakonczenia (w sekundach).
     * @param killedMonsters Opcjonalny licznik zabitych potworow.
     * @param destroyedMetins Opcjonalny licznik zniszczonych metinow.
     */
    DungeonStateUpdatedEvent(EterBase::DungeonId id,
                             std::optional<uint32_t> currentFloor,
                             std::optional<uint32_t> remainingTime,
                             std::optional<uint32_t> killedMonsters,
                             std::optional<uint32_t> destroyedMetins)
        : id(id),
          currentFloor(currentFloor),
          remainingTime(remainingTime),
          killedMonsters(killedMonsters),
          destroyedMetins(destroyedMetins) {}
};

/**
 * @brief Model utrzymujacy stan biezacej instancji lochu.
 */
class DungeonStateModel {
public:
    /**
     * @brief Konstruktor modelu stanu lochu.
     * @param id Identyfikator instancji lochu.
     */
    explicit DungeonStateModel(EterBase::DungeonId id) : id_(id) {
        EterBase::ModernLogger::Info("DungeonStateModel created for DungeonId: {}", id_.value());
    }

    /**
     * @brief Zwraca identyfikator lochu.
     * @return Identyfikator lochu typu DungeonId.
     */
    [[nodiscard]] EterBase::DungeonId GetId() const noexcept {
        return id_;
    }

    /**
     * @brief Zwraca aktualny poziom lochu, jesli ustawiony.
     * @return Aktualny poziom lochu (std::optional).
     */
    [[nodiscard]] std::optional<uint32_t> GetCurrentFloor() const noexcept {
        return currentFloor_;
    }

    /**
     * @brief Zwraca pozostaly czas, jesli ustawiony.
     * @return Pozostaly czas w sekundach (std::optional).
     */
    [[nodiscard]] std::optional<uint32_t> GetRemainingTime() const noexcept {
        return remainingTime_;
    }

    /**
     * @brief Zwraca ilosc zabitych potworow, jesli ustawiono.
     * @return Ilosc zabitych potworow (std::optional).
     */
    [[nodiscard]] std::optional<uint32_t> GetKilledMonsters() const noexcept {
        return killedMonsters_;
    }

    /**
     * @brief Zwraca ilosc zniszczonych metinow, jesli ustawiono.
     * @return Ilosc zniszczonych metinow (std::optional).
     */
    [[nodiscard]] std::optional<uint32_t> GetDestroyedMetins() const noexcept {
        return destroyedMetins_;
    }

    /**
     * @brief Ustawia poziom lochu i emituje zdarzenie.
     * @param floor Poziom lochu (pietro).
     * @return Oczekiwany brak bledu lub kod bledu PacketResult<void>.
     */
    EterBase::PacketResult<void> SetCurrentFloor(uint32_t floor) {
        if (floor == 0) {
            EterBase::ModernLogger::Error("Invalid dungeon floor: 0 for DungeonId: {}", id_.value());
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }

        currentFloor_ = floor;
        EterBase::ModernLogger::Debug("DungeonId {} floor updated to {}", id_.value(), floor);
        PublishUpdateEvent();
        return {};
    }

    /**
     * @brief Ustawia pozostaly czas lochu i emituje zdarzenie.
     * @param time Pozostaly czas w sekundach.
     * @return Oczekiwany brak bledu lub kod bledu PacketResult<void>.
     */
    EterBase::PacketResult<void> SetRemainingTime(uint32_t time) {
        remainingTime_ = time;
        EterBase::ModernLogger::Debug("DungeonId {} remaining time updated to {}", id_.value(), time);
        PublishUpdateEvent();
        return {};
    }

    /**
     * @brief Ustawia ilosc zabitych potworow w lochu i emituje zdarzenie.
     * @param count Ilosc zabitych potworow.
     * @return Oczekiwany brak bledu lub kod bledu PacketResult<void>.
     */
    EterBase::PacketResult<void> SetKilledMonsters(uint32_t count) {
        killedMonsters_ = count;
        EterBase::ModernLogger::Debug("DungeonId {} killed monsters updated to {}", id_.value(), count);
        PublishUpdateEvent();
        return {};
    }

    /**
     * @brief Ustawia ilosc zniszczonych metinow w lochu i emituje zdarzenie.
     * @param count Ilosc zniszczonych metinow.
     * @return Oczekiwany brak bledu lub kod bledu PacketResult<void>.
     */
    EterBase::PacketResult<void> SetDestroyedMetins(uint32_t count) {
        destroyedMetins_ = count;
        EterBase::ModernLogger::Debug("DungeonId {} destroyed metins updated to {}", id_.value(), count);
        PublishUpdateEvent();
        return {};
    }

    /**
     * @brief Zwieksza ilosc zabitych potworow o 1 i emituje zdarzenie.
     * @return Oczekiwany brak bledu lub kod bledu PacketResult<void>.
     */
    EterBase::PacketResult<void> IncrementKilledMonsters() {
        killedMonsters_ = killedMonsters_.value_or(0) + 1;
        EterBase::ModernLogger::Debug("DungeonId {} killed monsters incremented to {}", id_.value(), *killedMonsters_);
        PublishUpdateEvent();
        return {};
    }

    /**
     * @brief Zwieksza ilosc zniszczonych metinow o 1 i emituje zdarzenie.
     * @return Oczekiwany brak bledu lub kod bledu PacketResult<void>.
     */
    EterBase::PacketResult<void> IncrementDestroyedMetins() {
        destroyedMetins_ = destroyedMetins_.value_or(0) + 1;
        EterBase::ModernLogger::Debug("DungeonId {} destroyed metins incremented to {}", id_.value(), *destroyedMetins_);
        PublishUpdateEvent();
        return {};
    }

    /**
     * @brief Aktualizuje od razu caly stan lochu.
     * @param floor Poziom lochu.
     * @param remainingTime Pozostaly czas w sekundach.
     * @param killedMonsters Liczba zabitych potworow.
     * @param destroyedMetins Liczba zniszczonych metinow.
     * @return Oczekiwany brak bledu lub kod bledu PacketResult<void>.
     */
    EterBase::PacketResult<void> UpdateFullState(uint32_t floor, uint32_t remainingTime, uint32_t killedMonsters, uint32_t destroyedMetins) {
        if (floor == 0) {
            EterBase::ModernLogger::Error("Invalid dungeon floor: 0 for DungeonId: {}", id_.value());
            return std::unexpected(EterBase::PacketError::MalformedPayload);
        }
        currentFloor_ = floor;
        remainingTime_ = remainingTime;
        killedMonsters_ = killedMonsters;
        destroyedMetins_ = destroyedMetins;

        EterBase::ModernLogger::Debug("DungeonId {} full state updated: floor={}, time={}, monsters={}, metins={}",
                                      id_.value(), floor, remainingTime, killedMonsters, destroyedMetins);
        PublishUpdateEvent();
        return {};
    }

private:
    EterBase::DungeonId id_;
    std::optional<uint32_t> currentFloor_;
    std::optional<uint32_t> remainingTime_;
    std::optional<uint32_t> killedMonsters_;
    std::optional<uint32_t> destroyedMetins_;

    /**
     * @brief Pomocnicza metoda do rozglaszania zdarzenia aktualizacji do EventBusa.
     */
    void PublishUpdateEvent() const {
        Core::EventBus::Instance().Publish(DungeonStateUpdatedEvent(
            id_, currentFloor_, remainingTime_, killedMonsters_, destroyedMetins_
        ));
    }
};

} // namespace UserInterface::Domain
