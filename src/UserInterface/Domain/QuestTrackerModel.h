#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <cstdint>
#include <chrono>
#include <algorithm>
#include <optional>
#include <functional>
#include <expected>

#include "../Core/EventBus.h"
#include "../../EterBase/StrongTypes.h"
#include "../../EterBase/Result.h"
#include "../../EterBase/LogModern.h"

namespace UserInterface::Domain {

/**
 * @brief Identyfikator unikalny misji (questa) w systemie klienta.
 */
using QuestId = EterBase::StrongType<struct QuestIdTag, uint32_t, 0>;

/**
 * @brief Struktura przechowujaca instancje pojedynczego zadania (questa).
 * 
 * Hermetyzuje wszystkie dane potrzebne do wyswietlenia statusu misji na warstwie UI.
 */
struct QuestInstance {
    QuestId id;
    std::string title;
    std::string clockName;
    std::string counterName;
    std::string iconFileName;
    int clockValue{0};
    int counterValue{0};
    std::chrono::system_clock::time_point startTime;
};

/**
 * @brief Przestrzen nazw dla zdarzen zwiazanych ze zmianami stanu misji.
 * Pozwala to na calkowite odpiecie od warstwy GUI (Zero-Conflict & Event-Driven).
 */
namespace Events {

/**
 * @brief Zdarzenie wyzwalane przy dodaniu nowej misji.
 */
struct QuestAdded : public UserInterface::Core::IEvent {
    QuestId id;
    QuestAdded() = default;
    explicit QuestAdded(QuestId i) : id(i) {}
};

/**
 * @brief Zdarzenie wyzwalane przy usuwaniu misji.
 */
struct QuestRemoved : public UserInterface::Core::IEvent {
    QuestId id;
    QuestRemoved() = default;
    explicit QuestRemoved(QuestId i) : id(i) {}
};

/**
 * @brief Zdarzenie wyzwalane przy modyfikacji danych misji.
 */
struct QuestUpdated : public UserInterface::Core::IEvent {
    QuestId id;
    QuestUpdated() = default;
    explicit QuestUpdated(QuestId i) : id(i) {}
};

/**
 * @brief Zdarzenie wyzwalane przy usuwaniu wszystkich misji.
 */
struct QuestCleared : public UserInterface::Core::IEvent {};

} // namespace Events

/**
 * @brief Domena i model dla sledzenia misji po stronie klienta (Quest Tracker).
 *
 * Klasa implementuje nowoczesne standardy C++23, silne typy oraz monadyczne podejscie
 * do obslugi bledow bez archaicznych wskaznikow out-param. Wszystkie zmiany stanu
 * emitowane sa za pomoca UserInterface::Core::EventBus, pozostawiajac UI pasywnym obserwatorem.
 */
class QuestTrackerModel {
public:
    /**
     * @brief Zwraca instancje (Singleton) dla modulu QuestTrackerModel.
     * @return Referencja do singletonu.
     */
    static QuestTrackerModel& GetInstance() {
        static QuestTrackerModel instance;
        return instance;
    }

    QuestTrackerModel(const QuestTrackerModel&) = delete;
    QuestTrackerModel& operator=(const QuestTrackerModel&) = delete;
    QuestTrackerModel(QuestTrackerModel&&) = delete;
    QuestTrackerModel& operator=(QuestTrackerModel&&) = delete;

    /**
     * @brief Domyslny konstruktor.
     */
    QuestTrackerModel() = default;

    /**
     * @brief Domyslny destruktor.
     */
    ~QuestTrackerModel() = default;

    /**
     * @brief Calkowicie czysci zasoby modelu.
     */
    void Clear() {
        quests_.clear();
        UserInterface::Core::EventBus::GetInstance().Publish(Events::QuestCleared{});
        EterBase::ModernLogger::Info("QuestTrackerModel cleared all quests.");
    }

    /**
     * @brief Rejestruje badz aktualizuje cala strukture zadania w trackerze.
     * @param questInstance Nowa instancja wypelniona danymi serwerowymi.
     */
    void RegisterQuestInstance(QuestInstance questInstance) {
        DeleteQuestInstance(questInstance.id);
        questInstance.startTime = std::chrono::system_clock::now();
        quests_.push_back(std::move(questInstance));
        
        UserInterface::Core::EventBus::GetInstance().Publish(Events::QuestAdded{quests_.back().id});
        EterBase::ModernLogger::Debug("Registered quest id: {}", static_cast<uint32_t>(quests_.back().id));
    }

    /**
     * @brief Usuwa misje jezeli ta istnieje w dzienniku zadan.
     * @param id Silny typ identyfikatora zadania.
     */
    void DeleteQuestInstance(QuestId id) {
        auto it = std::ranges::find_if(quests_, [id](const QuestInstance& q) {
            return q.id == id;
        });

        if (it != quests_.end()) {
            quests_.erase(it);
            UserInterface::Core::EventBus::GetInstance().Publish(Events::QuestRemoved{id});
            EterBase::ModernLogger::Debug("Deleted quest id: {}", static_cast<uint32_t>(id));
        }
    }

    /**
     * @brief Weryfikuje istnienie misji.
     * @param id Silny typ identyfikatora zadania.
     * @return True jezeli misja istnieje w wektorze.
     */
    [[nodiscard]] bool IsQuest(QuestId id) const {
        return GetQuest(id).has_value();
    }

    /**
     * @brief Przygotowuje rezerwacje w pamieci pod nowe zadanie.
     * @param id Silny typ identyfikatora zadania.
     */
    void MakeQuest(QuestId id) {
        if (!IsQuest(id)) {
            QuestInstance newQuest{};
            newQuest.id = id;
            RegisterQuestInstance(std::move(newQuest));
        }
    }

    /**
     * @brief Ustawia nazwe (tytul) zadania w interfejsie uzytkownika.
     * @param id Identyfikator zadania.
     * @param title Ciag znakow zawierajacy pelen tytul zadania z locale.
     * @return EterBase::VoidResult informujacy o poprawnosci operacji.
     */
    EterBase::VoidResult<> SetQuestTitle(QuestId id, std::string_view title) {
        return GetQuestMut(id).transform([&](std::reference_wrapper<QuestInstance> quest) {
            quest.get().title = title;
            UserInterface::Core::EventBus::GetInstance().Publish(Events::QuestUpdated{id});
        });
    }

    /**
     * @brief Ustawia nazwe skryptowa UI odzwierciedlajaca timer zadania.
     * @param id Identyfikator zadania.
     * @param clockName Nazwa odwolania sie w warstwie Pythona/UI dla okienka Timera.
     * @return EterBase::VoidResult informujacy o poprawnosci operacji.
     */
    EterBase::VoidResult<> SetQuestClockName(QuestId id, std::string_view clockName) {
        return GetQuestMut(id).transform([&](std::reference_wrapper<QuestInstance> quest) {
            quest.get().clockName = clockName;
            UserInterface::Core::EventBus::GetInstance().Publish(Events::QuestUpdated{id});
        });
    }

    /**
     * @brief Ustawia nazwe skryptowa UI odzwierciedlajaca postep zadania (np ilosc mobow).
     * @param id Identyfikator zadania.
     * @param counterName Nazwa wyswietlana interfejsu (np. Potwory:, Pozostalo:).
     * @return EterBase::VoidResult informujacy o poprawnosci operacji.
     */
    EterBase::VoidResult<> SetQuestCounterName(QuestId id, std::string_view counterName) {
        return GetQuestMut(id).transform([&](std::reference_wrapper<QuestInstance> quest) {
            quest.get().counterName = counterName;
            UserInterface::Core::EventBus::GetInstance().Publish(Events::QuestUpdated{id});
        });
    }

    /**
     * @brief Aktualizuje logike zegara.
     * @param id Identyfikator zadania.
     * @param clockValue Ilosc sekund lub czas wyslany z serwera.
     * @return EterBase::VoidResult informujacy o poprawnosci operacji.
     */
    EterBase::VoidResult<> SetQuestClockValue(QuestId id, int clockValue) {
        return GetQuestMut(id).transform([&](std::reference_wrapper<QuestInstance> quest) {
            quest.get().clockValue = clockValue;
            quest.get().startTime = std::chrono::system_clock::now();
            UserInterface::Core::EventBus::GetInstance().Publish(Events::QuestUpdated{id});
        });
    }

    /**
     * @brief Aktualizuje logike licznika np. progres poleglych przeciwnikow.
     * @param id Identyfikator zadania.
     * @param counterValue Postep (np. ilosc zebranych przedmiotow).
     * @return EterBase::VoidResult informujacy o poprawnosci operacji.
     */
    EterBase::VoidResult<> SetQuestCounterValue(QuestId id, int counterValue) {
        return GetQuestMut(id).transform([&](std::reference_wrapper<QuestInstance> quest) {
            quest.get().counterValue = counterValue;
            UserInterface::Core::EventBus::GetInstance().Publish(Events::QuestUpdated{id});
        });
    }

    /**
     * @brief Zmienia grafike dla zadania.
     * @param id Identyfikator zadania.
     * @param iconFileName Wzgledna sciezka pliku w d:\ymir work... do ikonki wyswietlanej u klienta.
     * @return EterBase::VoidResult informujacy o poprawnosci operacji.
     */
    EterBase::VoidResult<> SetQuestIconFileName(QuestId id, std::string_view iconFileName) {
        return GetQuestMut(id).transform([&](std::reference_wrapper<QuestInstance> quest) {
            quest.get().iconFileName = iconFileName;
            UserInterface::Core::EventBus::GetInstance().Publish(Events::QuestUpdated{id});
        });
    }

    /**
     * @brief Zwraca liczbe zakolejkowanych/aktywnych zadan.
     * @return Rozmiar wewnetrznego rejestru sledzonych misji.
     */
    [[nodiscard]] size_t GetQuestCount() const {
        return quests_.size();
    }

    /**
     * @brief Uzyskuje referencje do danych zadania patrzac wylacznie na indeks kontenera.
     * Wymagane przez warstwe Pythona ktora renderuje questy uzywajac petli 0..N wylacznie do ich wylistowania.
     * @param index Indeks w wewnetrznej tablicy.
     * @return EterBase::Result z referencja read-only do obiektu zadania lub bledem przekroczenia rozmiaru.
     */
    [[nodiscard]] EterBase::Result<std::reference_wrapper<const QuestInstance>> GetQuestInstanceByIndex(size_t index) const {
        if (index < quests_.size()) {
            return std::cref(quests_[index]);
        }
        return std::unexpected(std::string_view{"Index out of range"});
    }

    /**
     * @brief Wyluskuje informacje wylacznie do wgladu dla zadania o podanym kluczu.
     * @param id Identyfikator weryfikowanej misji.
     * @return EterBase::Result z referencja read-only lub obiektywnym bledem logicznym brakujacego elementu.
     */
    [[nodiscard]] EterBase::Result<std::reference_wrapper<const QuestInstance>> GetQuest(QuestId id) const {
        auto it = std::ranges::find_if(quests_, [id](const QuestInstance& q) {
            return q.id == id;
        });

        if (it != quests_.end()) {
            return std::cref(*it);
        }
        return std::unexpected(std::string_view{"Quest not found"});
    }

private:
    /**
     * @brief Wewnetrzna funkcja operujaca monadycznie by pobierac referencje ze zgodnoscia uzycia modyfikatorow (mutate).
     * @param id Identyfikator weryfikowanej misji.
     * @return EterBase::Result z modyfikowalna referencja uzyteczne dla funkcji z Set*.
     */
    [[nodiscard]] EterBase::Result<std::reference_wrapper<QuestInstance>> GetQuestMut(QuestId id) {
        auto it = std::ranges::find_if(quests_, [id](const QuestInstance& q) {
            return q.id == id;
        });

        if (it != quests_.end()) {
            return std::ref(*it);
        }
        EterBase::ModernLogger::Warn("QuestTrackerModel: Quest id {} not found", static_cast<uint32_t>(id));
        return std::unexpected(std::string_view{"Quest not found"});
    }

    std::vector<QuestInstance> quests_;
};

} // namespace UserInterface::Domain
