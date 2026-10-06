/**
 * @file TargetRankSorter.h
 * @brief Definiuje klase TargetRankSorter odpowiedzialna za sortowanie celow 
 * w systemie walki na podstawie priorytetu (Boss > Metin > Enemy).
 * Zgodny ze standardem C++23.
 */

#pragma once

#include <cstdint>
#include <span>
#include <optional>
#include <functional>
#include <algorithm>
#include <vector>
#include <expected>

#include "ActorInstance.h"
#include "EterBase/StrongTypes.h"
#include "EterBase/LogModern.h"
#include "UserInterface/Core/EventBus.h"

namespace Metin2::CombatMath
{
    /**
     * @brief Kody bledow operacji sortowania.
     */
    enum class SorterError : uint8_t
    {
        None = 0,
        EmptyTargetList,
        InvalidContext
    };

    /**
     * @brief Zdarzenie emitowane po udanym posortowaniu listy celow.
     * 
     * Pozwala odpiac logike interfejsu (GUI) od silnika walki,
     * zgodnie ze wzorcem Event-Driven.
     */
    struct TargetRankSortedEvent : public UserInterface::Core::IEvent
    {
        std::vector<EterBase::EntityId> sortedTargetIds;

        /**
         * @brief Inicjalizuje zdarzenie z posortowana lista identyfikatorow.
         * @param targets Wektor identyfikatorow jednostek (silne typy EntityId).
         */
        explicit TargetRankSortedEvent(std::vector<EterBase::EntityId> targets)
            : sortedTargetIds(std::move(targets))
        {
        }
    };

    /**
     * @brief Kontekst wymagany do poprawnego dzialania sortowania celow.
     */
    struct SorterContext
    {
        /** @brief Callback sprawdzajacy, czy dana jednostka to Boss. */
        std::function<bool(CActorInstance*)> isBossCallback;
    };

    /**
     * @brief Klasa odpowiedzialna za sortowanie przeciwnikow wedlug priorytetu.
     * 
     * Priorytety: Boss > Metin (Kamien) > Moby (Zwykle potwory).
     */
    class TargetRankSorter
    {
    public:
        /**
         * @brief Domyslny konstruktor.
         */
        TargetRankSorter() = default;

        /**
         * @brief Domyslny destruktor.
         */
        ~TargetRankSorter() = default;

        /**
         * @brief Sortuje liste celow wedlug ustalonego priorytetu i emituje zdarzenie.
         * 
         * Wykorzystuje stabilne sortowanie (std::stable_sort).
         * Po zakonczeniu operacji emituje zdarzenie o zmianie rankingu.
         *
         * @param targets Span zawierajacy wskazniki na jednostki do posortowania.
         * @param context Kontekst zawierajacy zewnetrzne reguly (np. isBossCallback).
         * @return std::expected<void, SorterError> Sukces (void) lub kod bledu SorterError.
         */
        std::expected<void, SorterError> SortTargets(std::span<CActorInstance*> targets, const SorterContext& context) const
        {
            if (targets.empty())
            {
                EterBase::ModernLogger::Warn("TargetRankSorter: Cannot sort an empty target list.");
                return std::unexpected(SorterError::EmptyTargetList);
            }

            if (!context.isBossCallback)
            {
                EterBase::ModernLogger::Error("TargetRankSorter: Invalid context provided (missing isBossCallback).");
                return std::unexpected(SorterError::InvalidContext);
            }

            std::stable_sort(targets.begin(), targets.end(),
                [&context, this](CActorInstance* a, CActorInstance* b) -> bool
                {
                    return this->GetPriority(a, context).value_or(0) > this->GetPriority(b, context).value_or(0);
                });

            std::vector<EterBase::EntityId> sortedIds;
            sortedIds.reserve(targets.size());
            
            for (auto* instance : targets)
            {
                if (instance)
                {
                    sortedIds.emplace_back(instance->GetVirtualID());
                }
            }

            // Emituj zdarzenia przez Core::EventBus::Instance().Publish(...) 
            UserInterface::Core::EventBus::GetInstance().Publish(TargetRankSortedEvent{std::move(sortedIds)});

            EterBase::ModernLogger::Info("TargetRankSorter: Successfully sorted {} targets.", targets.size());
            return {};
        }

    private:
        /**
         * @brief Oblicza liczbowy priorytet jednostki do sortowania.
         * 
         * Wartosci:
         * 3 - Boss
         * 2 - Kamien Metin (TYPE_STONE)
         * 1 - Potwor (TYPE_ENEMY)
         * 0 - Inne
         * 
         * Wykorzystuje monadyczne operacje na std::optional.
         *
         * @param instance Wskaznik na jednostke.
         * @param context Kontekst dostarczajacy callback dla weryfikacji Bossa.
         * @return std::optional<int> Priorytet liczbowy, std::nullopt gdy wskaznik to nullptr.
         */
        std::optional<int> GetPriority(CActorInstance* instance, const SorterContext& context) const
        {
            return (instance ? std::make_optional(instance) : std::nullopt)
                .transform([&context](CActorInstance* act) -> int {
                    if (context.isBossCallback(act))
                    {
                        return 3;
                    }
                    if (act->GetActorType() == CActorInstance::TYPE_STONE)
                    {
                        return 2;
                    }
                    if (act->GetActorType() == CActorInstance::TYPE_ENEMY)
                    {
                        return 1;
                    }
                    return 0;
                });
        }
    };
} // namespace Metin2::CombatMath
