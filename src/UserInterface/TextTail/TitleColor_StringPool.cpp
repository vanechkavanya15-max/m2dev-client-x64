#include "../StdAfx.h"
#include "ITitleNameColorizer.h"
#include "../../EterBase/LogModern.h"
#include <string>
#include <unordered_set>
#include <shared_mutex>
#include <format>
#include <string_view>

namespace UserInterface::TextTail
{
    /**
     * @class TitleColor_StringPool
     * @brief Thread-safe string pool implementation for ITitleNameColorizer 
     *        using C++23 features and Zero-Conflict rules.
     */
    class TitleColor_StringPool : public ITitleNameColorizer
    {
    public:
        TitleColor_StringPool()
        {
            EterBase::ModernLogger::Info("TitleColor_StringPool initialized.");
        }

        ~TitleColor_StringPool() override
        {
            EterBase::ModernLogger::Info("TitleColor_StringPool destroyed.");
        }

        /**
         * @brief Gets the color for a specific alignment.
         * @param alignment The alignment value.
         * @return The ARGB color.
         */
        [[nodiscard]] uint32_t GetAlignmentColor(int32_t alignment) const override
        {
            return 0xFFFFFFFF; // Returning default color, logic unknown without context
        }

        /**
         * @brief Gets the color for a specific empire.
         * @param empire The empire ID.
         * @return The ARGB color.
         */
        [[nodiscard]] uint32_t GetEmpireColor(uint8_t empire) const override
        {
            return 0xFFFFFFFF; // Returning default color, logic unknown without context
        }

        /**
         * @brief Gets the color for the level difference between player and mob.
         * @param playerLevel The player's level.
         * @param mobLevel The mob's level.
         * @return The ARGB color.
         */
        [[nodiscard]] uint32_t GetLevelColor(int32_t playerLevel, int32_t mobLevel) const override
        {
            return 0xFFFFFFFF; // Returning default color, logic unknown without context
        }

        /**
         * @brief Formats and pools a guild name.
         * @param guildName The raw guild name.
         * @return The formatted guild name.
         */
        [[nodiscard]] std::string FormatGuildName(std::string_view guildName) const override
        {
            if (guildName.empty())
                return "";
                
            return InternString(std::string(guildName));
        }

        /**
         * @brief Formats and pools an alignment title.
         * @param alignment The alignment value.
         * @return The formatted alignment string.
         */
        [[nodiscard]] std::string FormatAlignmentTitle(int32_t alignment) const override
        {
            return InternString(std::to_string(alignment));
        }

        /**
         * @brief Clears the string pool.
         */
        void Clear() override
        {
            std::unique_lock lock(m_mutex);
            size_t count = m_pool.size();
            m_pool.clear();
            EterBase::ModernLogger::Info("TitleColor_StringPool cleared ({} strings removed).", count);
        }

    private:
        /**
         * @brief Interns a string to avoid duplication.
         * @param str The string to intern.
         * @return The interned string.
         */
        std::string InternString(const std::string& str) const
        {
            {
                std::shared_lock readLock(m_mutex);
                if (auto it = m_pool.find(str); it != m_pool.end())
                {
                    return *it;
                }
            }

            std::unique_lock writeLock(m_mutex);
            auto [it, inserted] = m_pool.insert(str);
            return *it;
        }

        mutable std::shared_mutex m_mutex;
        mutable std::unordered_set<std::string> m_pool;
    };
}
