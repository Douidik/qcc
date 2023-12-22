#ifndef QCC_COMMON_HPP
#define QCC_COMMON_HPP

#include "error.hpp"
#include <cstdint>
#include <fmt/core.h>
#include <ranges>
#include <source_location>

namespace qcc
{

namespace views = std::views;
namespace ranges = std::ranges;

typedef uint32_t uint;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef __int128 int128;

typedef float float32;
typedef double float64;
typedef long double float128;

constexpr size_t npos = (size_t)-1;

#ifdef NDEBUG
#define QCC_RELEASE
#else
#define QCC_DEBUG
#endif

static inline void qcc_assert(std::string_view message, bool boolean)
{
    [[unlikely]] if (!boolean) {
        fmt::print(stderr, "internal qcc assertion failed: {}\n", message);
        abort();
    }
}

// static inline void qcc_assert(std::string_view message, bool boolean,
//                        std::source_location source_location = std::source_location::current())
// {
//     [[unlikely]] if (!boolean) {
//         std::string_view file = source_location.file_name();
//         std::string_view function = source_location.function_name();
//         uint32 line = source_location.line();
//         fmt::print(stderr, "internal qcc assertion failed ({}:{}:{}): {}\n", file, function, line,
//         message);
// 	    abort();
//     }
// }

#define Bit(type, n) ((type)1 << n)

} // namespace qcc

#endif
