#pragma once

#include <cstdint>
#include <string_view>
#include <span>
#include <algorithm>

namespace EterBase::StringUtils
{
    /**
     * @brief Checks if a character is a whitespace.
     * @param ch Character to check.
     * @return true if whitespace, false otherwise.
     */
    constexpr bool IsSpace(char ch) noexcept
    {
        return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\v' || ch == '\f';
    }

    /**
     * @brief Trims whitespaces from the left side of the string view.
     * @param str The source string view.
     * @return A new string view with left whitespaces removed.
     */
    constexpr std::string_view TrimLeft(std::string_view str) noexcept
    {
        std::size_t start = 0;
        while (start < str.length() && IsSpace(str[start]))
        {
            ++start;
        }
        return str.substr(start);
    }

    /**
     * @brief Trims whitespaces from the right side of the string view.
     * @param str The source string view.
     * @return A new string view with right whitespaces removed.
     */
    constexpr std::string_view TrimRight(std::string_view str) noexcept
    {
        std::size_t end = str.length();
        while (end > 0 && IsSpace(str[end - 1]))
        {
            --end;
        }
        return str.substr(0, end);
    }

    /**
     * @brief Trims whitespaces from both sides of the string view.
     * @param str The source string view.
     * @return A new string view with whitespaces removed from both sides.
     */
    constexpr std::string_view Trim(std::string_view str) noexcept
    {
        return TrimRight(TrimLeft(str));
    }

    /**
     * @brief A non-allocating iterator for splitting string views.
     */
    class SplitIterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::string_view;
        using difference_type = std::ptrdiff_t;
        using pointer = const std::string_view*;
        using reference = const std::string_view&;

        /**
         * @brief Default constructor for an end iterator.
         */
        constexpr SplitIterator() noexcept = default;

        /**
         * @brief Constructs an iterator for a string view and delimiter.
         * @param str The string view to split.
         * @param delimiter The delimiter character.
         */
        constexpr SplitIterator(std::string_view str, char delimiter) noexcept
            : str(str), delimiter(delimiter)
        {
            FindNext();
        }

        /**
         * @brief Dereferences the iterator.
         * @return The current token string view.
         */
        constexpr reference operator*() const noexcept
        {
            return current_token;
        }

        /**
         * @brief Dereferences the iterator.
         * @return Pointer to the current token string view.
         */
        constexpr pointer operator->() const noexcept
        {
            return &current_token;
        }

        /**
         * @brief Advances the iterator to the next token.
         * @return Reference to the updated iterator.
         */
        constexpr SplitIterator& operator++() noexcept
        {
            if (end_reached)
            {
                str = {};
                return *this;
            }
            str = str.substr(current_token.length());
            if (!str.empty())
            {
                str = str.substr(1); // skip delimiter
            }
            FindNext();
            return *this;
        }

        /**
         * @brief Advances the iterator to the next token.
         * @return The previous iterator state.
         */
        constexpr SplitIterator operator++(int) noexcept
        {
            SplitIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        /**
         * @brief Compares two iterators for equality.
         * @param other The other iterator.
         * @return true if equal, false otherwise.
         */
        constexpr bool operator==(const SplitIterator& other) const noexcept
        {
            return str.data() == other.str.data() && str.size() == other.str.size();
        }

        /**
         * @brief Compares two iterators for inequality.
         * @param other The other iterator.
         * @return true if not equal, false otherwise.
         */
        constexpr bool operator!=(const SplitIterator& other) const noexcept
        {
            return !(*this == other);
        }

    private:
        std::string_view str{};
        char delimiter{};
        std::string_view current_token{};
        bool end_reached{true};

        constexpr void FindNext() noexcept
        {
            if (str.data() == nullptr)
            {
                end_reached = true;
                return;
            }
            
            std::size_t pos = str.find(delimiter);
            if (pos == std::string_view::npos)
            {
                current_token = str;
                end_reached = true;
            }
            else
            {
                current_token = str.substr(0, pos);
                end_reached = false;
            }
        }
    };

    /**
     * @brief A non-allocating view for splitting string views.
     */
    class SplitView
    {
    public:
        /**
         * @brief Constructs a split view.
         * @param str The string view to split.
         * @param delimiter The delimiter character.
         */
        constexpr SplitView(std::string_view str, char delimiter) noexcept
            : str(str), delimiter(delimiter) {}

        /**
         * @brief Returns an iterator to the beginning.
         * @return The begin iterator.
         */
        constexpr SplitIterator begin() const noexcept
        {
            return SplitIterator(str, delimiter);
        }

        /**
         * @brief Returns an iterator to the end.
         * @return The end iterator.
         */
        constexpr SplitIterator end() const noexcept
        {
            return SplitIterator();
        }

    private:
        std::string_view str;
        char delimiter;
    };

    /**
     * @brief Lazily splits a string view into tokens.
     * @param str The string view to split.
     * @param delimiter The character to split by.
     * @return A SplitView that can be iterated over.
     */
    constexpr SplitView Split(std::string_view str, char delimiter) noexcept
    {
        return SplitView(str, delimiter);
    }

    /**
     * @brief Converts a buffer of characters to lowercase in-place.
     * @param buffer The span of characters to modify.
     */
    constexpr void ToLower(std::span<char> buffer) noexcept
    {
        for (char& ch : buffer)
        {
            if (ch >= 'A' && ch <= 'Z')
            {
                ch = static_cast<char>(ch + ('a' - 'A'));
            }
        }
    }

    /**
     * @brief Writes a lowercase version of a string view into a destination span.
     *        Writes up to the minimum of the source and destination sizes.
     * @param source The source string view.
     * @param destination The output span to write characters to.
     * @return The number of characters written.
     */
    constexpr std::size_t ToLower(std::string_view source, std::span<char> destination) noexcept
    {
        std::size_t count = std::min(source.size(), destination.size());
        for (std::size_t i = 0; i < count; ++i)
        {
            char ch = source[i];
            if (ch >= 'A' && ch <= 'Z')
            {
                destination[i] = static_cast<char>(ch + ('a' - 'A'));
            }
            else
            {
                destination[i] = ch;
            }
        }
        return count;
    }
}
