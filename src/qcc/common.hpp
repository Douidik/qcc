#ifndef QCC_COMMON_HPP
#define QCC_COMMON_HPP

#include "error.hpp"
#include <cstdint>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <iterator>
#include <ranges>
#include <source_location>

namespace qcc
{
namespace fs = std::filesystem;
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

[[noreturn]]
static inline void qcc_print_crash(std::string_view context, std::string_view message,
                                   std::source_location source_location)
{
    fs::path file = source_location.file_name();
    std::string_view function = source_location.function_name();
    uint32 line = source_location.line();
    fmt::print(stderr, "{} ({}:{}:{}): {}\n", context, file.filename().string(), function, line, message);
    abort();
}

static inline void qcc_assert(bool boolean, std::string_view message = "?",
                              std::source_location source_location = std::source_location::current())
{
    [[unlikely]] if (!boolean) {
        qcc_print_crash("assertion failed!", message, source_location);
    }
}

[[noreturn]]
static inline void qcc_todo(std::string_view message = "?",
                            std::source_location source_location = std::source_location::current())
{
    qcc_print_crash("todo!", message, source_location);
}

static inline std::string fstream_to_str(std::fstream &&fstream)
{
    qcc_assert(fstream.is_open(), "error occured on stream");
    std::string str = {std::istreambuf_iterator(fstream), {}};
    str.push_back('\n');
    fstream.close();
    return str;
}

#define Round_Up(x, y) ((x + y - 1) & ~(y - 1))
#define Bit(type, n) ((type)1 << n)

} // namespace qcc

#endif
