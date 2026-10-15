#pragma once

#include "Debug.h"
#include "LogModern.h"

namespace EterBase {

/**
 * @brief Pomocnicze aliasy szablonow std::format w namespace EterBase
 */
template <typename... Args>
inline void TraceError(std::format_string<Args...> fmt, Args&&... args)
{
    ::TraceErrorFmt(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void Trace(std::format_string<Args...> fmt, Args&&... args)
{
    ::TraceFmt(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void Log(UINT uLevel, std::format_string<Args...> fmt, Args&&... args)
{
    ::LogFmt(uLevel, fmt, std::forward<Args>(args)...);
}

} // namespace EterBase
