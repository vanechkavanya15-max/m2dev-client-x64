#pragma once

#include <cstdint>
#include <array>
#include <bit>
#include <expected>
#include <string_view>
#include <d3d9.h>

namespace EterLib::Render {

/**
 * @brief Bitmask for efficiently tracking dirty or active render states up to 256.
 */
class StateBitmask256 {
public:
    constexpr void Set(size_t bit) noexcept {
        if (bit < 256) {
            m_mask[bit / 64] |= (1ULL << (bit % 64));
        }
    }

    constexpr void Clear(size_t bit) noexcept {
        if (bit < 256) {
            m_mask[bit / 64] &= ~(1ULL << (bit % 64));
        }
    }

    [[nodiscard]] constexpr bool Test(size_t bit) const noexcept {
        if (bit < 256) {
            return (m_mask[bit / 64] & (1ULL << (bit % 64))) != 0;
        }
        return false;
    }

    constexpr void ClearAll() noexcept {
        m_mask.fill(0);
    }
    
    [[nodiscard]] constexpr size_t Count() const noexcept {
        size_t count = 0;
        for (auto val : m_mask) {
            count += std::popcount(val);
        }
        return count;
    }

private:
    std::array<uint64_t, 4> m_mask{};
};

/**
 * @brief Fixed capacity stack avoiding dynamic allocation for render states.
 */
template <typename T, size_t MaxDepth = 16>
class FixedStateStack {
public:
    constexpr bool Push(const T& value) noexcept {
        if (m_size >= MaxDepth) {
            return false;
        }
        m_stack[m_size++] = value;
        return true;
    }

    constexpr std::expected<T, std::string_view> Pop() noexcept {
        if (m_size == 0) {
            return std::unexpected("Stack underflow");
        }
        return m_stack[--m_size];
    }

    [[nodiscard]] constexpr size_t Size() const noexcept {
        return m_size;
    }

    constexpr void Clear() noexcept {
        m_size = 0;
    }
    
    [[nodiscard]] constexpr bool IsEmpty() const noexcept {
        return m_size == 0;
    }

private:
    std::array<T, MaxDepth> m_stack{};
    size_t m_size{0};
};

/**
 * @brief Bridge bridging FixedStateStack and StateBitmask256 with CStateManager.
 */
class ModernStateManagerBridge {
public:
    static ModernStateManagerBridge& Instance();

    void BeginFrame();
    void EndFrame();

    void SetRenderState(D3DRENDERSTATETYPE state, DWORD value);
    void SaveRenderState(D3DRENDERSTATETYPE state, DWORD value);
    void RestoreRenderState(D3DRENDERSTATETYPE state);

    [[nodiscard]] size_t GetStateSwitchCount() const noexcept;
    [[nodiscard]] size_t GetActiveStateCount() const noexcept;
    [[nodiscard]] bool IsStateActive(D3DRENDERSTATETYPE state) const noexcept;
    
    void Reset();

private:
    ModernStateManagerBridge() = default;
    ~ModernStateManagerBridge() = default;

    ModernStateManagerBridge(const ModernStateManagerBridge&) = delete;
    ModernStateManagerBridge& operator=(const ModernStateManagerBridge&) = delete;

    std::array<FixedStateStack<DWORD, 16>, 256> m_stacks{};
    StateBitmask256 m_activeStates;
    size_t m_stateSwitches{0};
};

} // namespace EterLib::Render
