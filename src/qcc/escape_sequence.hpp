#ifndef QCC_ESCAPE_SEQUENCE_HPP
#define QCC_ESCAPE_SEQUENCE_HPP

#include "common.hpp"
#include <string>

namespace qcc
{

constexpr std::string_view escape_sequence(const char &c)
{
    switch (c) {
    case '\t':
        return R"(\t)";
    case '\v':
        return R"(\v)";
    case '\0':
        return R"(\0)";
    case '\b':
        return R"(\b)";
    case '\f':
        return R"(\f)";
    case '\n':
        return R"(\n)";
    case '\r':
        return R"(\r)";
    case '\\':
        return R"(\\)";
    case '\"':
        return R"(\")";
    default:
        return std::string_view{&c, 1};
    }
}

// NOTE! We assume the previous character is '\'
constexpr char un_escape_sequence(const char &c)
{
    switch (c) {
    case 't':
        return '\t';
    case 'v':
        return '\v';
    case '0':
        return '\0';
    case 'b':
        return '\b';
    case 'f':
        return '\f';
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case '"':
        return '"';
    default:
        throw Error{"un_escape_sequence() error", "unknown escape sequence"};
    }
}

std::string escape_string(std::string_view s, int depth = 1);
std::string un_escape_string(std::string_view s, int depth = 1);

} // namespace bee

#endif
