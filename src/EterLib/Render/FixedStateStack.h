#pragma once

#include <array>
#include <cstddef>
#include <utility>

namespace EterLib::Render
{
    template <typename T, std::size_t MaxDepth = 8>
    class FixedStateStack
    {
    public:
        constexpr FixedStateStack() noexcept = default;
        ~FixedStateStack() = default;

        FixedStateStack(const FixedStateStack&) = default;
        FixedStateStack& operator=(const FixedStateStack&) = default;
        FixedStateStack(FixedStateStack&&) = default;
        FixedStateStack& operator=(FixedStateStack&&) = default;

        void Push(T val) noexcept
        {
            if (m_depth < MaxDepth)
            {
                m_data[m_depth] = std::move(val);
                ++m_depth;
            }
        }

        [[nodiscard]] T Pop() noexcept
        {
            if (m_depth > 0)
            {
                --m_depth;
                T val = std::move(m_data[m_depth]);
                m_data[m_depth] = T{}; // reset to default to avoid holding resources
                return val;
            }
            return T{};
        }

        [[nodiscard]] T Top() const noexcept
        {
            if (m_depth > 0)
            {
                return m_data[m_depth - 1];
            }
            return T{};
        }

        [[nodiscard]] std::size_t Depth() const noexcept
        {
            return m_depth;
        }

        [[nodiscard]] bool IsEmpty() const noexcept
        {
            return m_depth == 0;
        }

        void Clear() noexcept
        {
            for (std::size_t i = 0; i < m_depth; ++i)
            {
                m_data[i] = T{};
            }
            m_depth = 0;
        }

    private:
        std::array<T, MaxDepth> m_data{};
        std::size_t m_depth{0};
    };
}
