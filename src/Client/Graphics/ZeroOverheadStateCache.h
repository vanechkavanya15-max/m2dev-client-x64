#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

struct IDirect3DDevice9;

namespace Client::Graphics
{
    /**
     * @brief Bezalokacyjny szablon stosu o stalym rozmiarze (zero sterty, zero std::vector).
     * Uzywany glownie do zapamietywania poprzednich stanow renderowania przy zagniezdzonym renderowaniu UI.
     */
    template <typename T, size_t Capacity = 16>
    class FixedStack
    {
    public:
        static_assert(Capacity > 0, "Pojemnosc stosu musi byc wieksza niz 0");

        constexpr FixedStack() noexcept = default;

        constexpr bool Push(const T& value) noexcept
        {
            if (m_size < Capacity)
            {
                m_data[m_size++] = value;
                return true;
            }
            return false;
        }

        constexpr bool Pop(T& outValue) noexcept
        {
            if (m_size > 0)
            {
                --m_size;
                outValue = m_data[m_size];
                return true;
            }
            return false;
        }

        constexpr bool Pop() noexcept
        {
            if (m_size > 0)
            {
                --m_size;
                return true;
            }
            return false;
        }

        [[nodiscard]] constexpr const T* Top() const noexcept
        {
            if (m_size > 0)
            {
                return &m_data[m_size - 1];
            }
            return nullptr;
        }

        [[nodiscard]] constexpr T* Top() noexcept
        {
            if (m_size > 0)
            {
                return &m_data[m_size - 1];
            }
            return nullptr;
        }

        [[nodiscard]] constexpr size_t Size() const noexcept { return m_size; }
        [[nodiscard]] constexpr size_t MaxCapacity() const noexcept { return Capacity; }
        [[nodiscard]] constexpr bool IsEmpty() const noexcept { return m_size == 0; }
        [[nodiscard]] constexpr bool IsFull() const noexcept { return m_size >= Capacity; }
        constexpr void Clear() noexcept { m_size = 0; }

    private:
        std::array<T, Capacity> m_data{};
        size_t m_size{0};
    };

    /**
     * @brief Struktura callbackow wywolan sprzetowych dla celow testowych / interceptora bez aktywnego okna D3D.
     */
    struct StateDispatchCallbacks
    {
        void (*onSetRenderState)(uint32_t stateType, uint32_t value, void* userData) = nullptr;
        void (*onSetTextureStageState)(uint32_t stage, uint32_t stateType, uint32_t value, void* userData) = nullptr;
        void (*onSetSamplerState)(uint32_t stage, uint32_t stateType, uint32_t value, void* userData) = nullptr;
        void* userData = nullptr;
    };

    /**
     * @brief Zero-Overhead State Cache dla renderera Direct3D 9 w standardzie C++23.
     * Buforuje stany renderowania, etapy tekstur i samplery w plaskich tablicach std::array,
     * calkowicie eliminujac zbedne wywolania API do GPU oraz blokady mutex i alokacje.
     */
    class ZeroOverheadStateCache
    {
    public:
        static constexpr size_t NUM_RENDER_STATES = 256;
        static constexpr size_t NUM_TEXTURE_STAGES = 8;
        static constexpr size_t NUM_STAGE_STATES = 64;
        static constexpr size_t NUM_SAMPLER_STATES = 16;
        static constexpr size_t STACK_DEPTH = 16;

        ZeroOverheadStateCache() noexcept;
        ~ZeroOverheadStateCache() noexcept = default;

        ZeroOverheadStateCache(const ZeroOverheadStateCache&) = delete;
        ZeroOverheadStateCache& operator=(const ZeroOverheadStateCache&) = delete;
        ZeroOverheadStateCache(ZeroOverheadStateCache&&) noexcept = default;
        ZeroOverheadStateCache& operator=(ZeroOverheadStateCache&&) noexcept = default;

        // Powiazanie urzadzenia D3D9 i callbackow
        void BindDevice(IDirect3DDevice9* device) noexcept;
        [[nodiscard]] IDirect3DDevice9* GetDevice() const noexcept;
        void SetCallbacks(const StateDispatchCallbacks& callbacks) noexcept;
        void ClearCallbacks() noexcept;

        // Render States (0..255)
        bool SetRenderState(uint32_t stateType, uint32_t value) noexcept;
        [[nodiscard]] uint32_t GetRenderState(uint32_t stateType) const noexcept;
        void PushRenderState(uint32_t stateType, uint32_t newValue) noexcept;
        void PopRenderState(uint32_t stateType) noexcept;

        // Texture Stage States (8 etapow x 64 stany)
        bool SetTextureStageState(uint32_t stage, uint32_t stateType, uint32_t value) noexcept;
        [[nodiscard]] uint32_t GetTextureStageState(uint32_t stage, uint32_t stateType) const noexcept;

        // Sampler States (8 etapow x 16 stanow)
        bool SetSamplerState(uint32_t stage, uint32_t stateType, uint32_t value) noexcept;
        [[nodiscard]] uint32_t GetSamplerState(uint32_t stage, uint32_t stateType) const noexcept;

        // Resetowanie i uniewaznianie
        void ResetToDefaults() noexcept;
        void InvalidateAll() noexcept;

        // Zarzadzanie maskami brudnych stanow (Dirty Bits)
        [[nodiscard]] uint64_t GetDirtyBlock(size_t blockIndex) const noexcept;
        [[nodiscard]] bool IsRenderStateDirty(uint32_t stateType) const noexcept;
        void ClearDirtyRenderState(uint32_t stateType) noexcept;
        void ClearAllDirty() noexcept;
        void MarkRenderStateDirty(uint32_t stateType) noexcept;
        void MarkAllDirty() noexcept;
        [[nodiscard]] bool HasAnyDirtyStates() const noexcept;

        // Telemetria wydajnosciowa
        [[nodiscard]] uint64_t GetFilteredCalls() const noexcept;
        [[nodiscard]] uint64_t GetForwardedCalls() const noexcept;
        [[nodiscard]] double GetEfficiencyRatio() const noexcept;
        void ResetTelemetry() noexcept;

    private:
        // Plaskie tablice stanow - zero std::unordered_map
        std::array<uint32_t, NUM_RENDER_STATES> m_renderStates{};
        std::array<std::array<uint32_t, NUM_STAGE_STATES>, NUM_TEXTURE_STAGES> m_textureStageStates{};
        std::array<std::array<uint32_t, NUM_SAMPLER_STATES>, NUM_TEXTURE_STAGES> m_samplerStates{};

        // 64-bitowa maska brudnych stanow: 4 bloki x 64 bity = 256 stanow
        uint64_t m_dirtyRenderStates[4]{};

        // Maski walidacji stanu
        uint64_t m_validRenderStates[4]{};
        std::array<uint64_t, NUM_TEXTURE_STAGES> m_validTextureStageStates{};
        std::array<uint16_t, NUM_TEXTURE_STAGES> m_validSamplerStates{};

        // Stosy Push/Pop dla kazdego ze stanow renderowania (zero alokacji sterty)
        std::array<FixedStack<uint32_t, STACK_DEPTH>, NUM_RENDER_STATES> m_renderStateStacks{};

        // Wskaznik na urzadzenie D3D9 i opcjonalne callbacki
        IDirect3DDevice9* m_device{nullptr};
        StateDispatchCallbacks m_callbacks{};

        // Statystyki telemetryczne
        uint64_t m_filteredCalls{0};
        uint64_t m_forwardedCalls{0};
    };
}
