#pragma once

#include "UserInterface/Contracts/IActorProvider.h"
#include <cstdint>

// Deklaracja wyprzedzajaca
class CInstanceBase;

namespace UserInterface::Actors::Subsystems
{
    class ActorRegistry;
    class CharacterPicker;

    /**
     * @brief Adapter dostarczajacy bezpieczny dostep do instancji aktorow.
     * 
     * Implementuje interfejs kontraktowy UserInterface::Contracts::IActorProvider.
     * Pozwala kontrolerom postaci gracza (PythonPlayer), wejsciu i logice
     * na bezpieczne odpytywanie o stan postaci bez bezposredniego wiazania
     * z pamiecia CPythonCharacterManager.
     * 
     * Guardraile:
     * 1. DANGLING POINTER SAFETY: Pelna weryfikacja istnienia instancji.
     *    Zapytania bezpieczne (IsActorAlive, GetActorPosition, GetDistance)
     *    nie wyciagaja wskaznika poza obreb adaptera.
     * 2. SINGLE THREADED: Calosc dziala w jednym watku renderu/gry,
     *    calkowity brak blokad i muteksow.
     */
    class ActorProviderAdapter : public UserInterface::Contracts::IActorProvider
    {
    public:
        ActorProviderAdapter() = default;
        explicit ActorProviderAdapter(const ActorRegistry* pRegistry, const CharacterPicker* pPicker = nullptr) noexcept;
        ~ActorProviderAdapter() override = default;

        ActorProviderAdapter(const ActorProviderAdapter&) = delete;
        ActorProviderAdapter& operator=(const ActorProviderAdapter&) = delete;
        ActorProviderAdapter(ActorProviderAdapter&&) noexcept = default;
        ActorProviderAdapter& operator=(ActorProviderAdapter&&) noexcept = default;

        // Implementacja metod IActorProvider:

        /**
         * @brief Pobranie instancji na biezaca klatke (ZAKAZ CACHOWANIA WSKAZNIKA).
         */
        [[nodiscard]] CInstanceBase* GetInstance(uint32_t vid) const override;
        [[nodiscard]] CInstanceBase* GetMainActor() const override;
        [[nodiscard]] CInstanceBase* GetPickedActor() const override;

        [[nodiscard]] bool IsActorAlive(uint32_t vid) const override;
        [[nodiscard]] bool GetActorPosition(uint32_t vid, TPixelPosition* outPos) const override;
        [[nodiscard]] float GetDistance(uint32_t vid1, uint32_t vid2) const override;
        [[nodiscard]] bool IsSamePartyMember(uint32_t vid1, uint32_t vid2) const override;

        // Metody konfiguracji adaptera:

        void SetRegistry(const ActorRegistry* pRegistry) noexcept;
        [[nodiscard]] const ActorRegistry* GetRegistry() const noexcept;

        void SetPicker(const CharacterPicker* pPicker) noexcept;
        [[nodiscard]] const CharacterPicker* GetPicker() const noexcept;

        void SetMainActorVID(uint32_t vid) noexcept;
        [[nodiscard]] uint32_t GetMainActorVID() const noexcept;

        void SetMainActorVid(EntityVid vid) noexcept { SetMainActorVID(vid.get()); }
        [[nodiscard]] EntityVid GetMainActorVid() const noexcept { return EntityVid(m_mainActorVID); }

    private:
        const ActorRegistry* m_pRegistry{nullptr};
        const CharacterPicker* m_pPicker{nullptr};
        uint32_t m_mainActorVID{0};
    };
}

namespace UserInterface::Actors
{
    using Subsystems::ActorProviderAdapter;
}
