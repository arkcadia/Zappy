#pragma once
#include <cstdio>
#include <format>
#include <string_view>

#ifdef VERBOSE
    #ifndef DEBUG
        #define DEBUG
    #endif
    inline constexpr bool kVerbose = true;
#else
    inline constexpr bool kVerbose = false;
#endif

#ifdef DEBUG
    inline constexpr bool kDebug = true;
#else
    inline constexpr bool kDebug = kVerbose;
#endif

namespace detail {

template <bool Enabled>
inline void log(std::string_view msg) noexcept {
    if constexpr (Enabled) {
        std::fputs(msg.data(), stderr);
        std::fputc('\n', stderr);
    }
}

template <bool Enabled, typename... Args>
inline void logf(std::format_string<Args...> fmt, Args&&... args) noexcept {
    if constexpr (Enabled) {
        const auto msg = std::format(fmt, std::forward<Args>(args)...);
        std::fputs(msg.c_str(), stderr);
        std::fputc('\n', stderr);
    }
}

} // namespace detail

// debug_logf("value = {}", 42)
template <typename... Args>
inline void debug_logf(std::format_string<Args...> fmt, Args&&... args) noexcept {
    detail::logf<kDebug>(fmt, std::forward<Args>(args)...);
}

// verbose_logf("value = {}", 42)
template <typename... Args>
inline void verbose_logf(std::format_string<Args...> fmt, Args&&... args) noexcept {
    detail::logf<kVerbose>(fmt, std::forward<Args>(args)...);
}

struct DebugModeAnnouncer {
    DebugModeAnnouncer() noexcept {
        if constexpr (kVerbose) { std::fputs("VERBOSE MODE\n", stderr); }
        else if constexpr (kDebug) { std::fputs("DEBUG MODE\n",   stderr); }
    }
};
