#ifndef QCC_REGEX_STATE_HPP
#define QCC_REGEX_STATE_HPP

#include "common.hpp"

namespace qcc::regex
{

struct Node;

enum Option : uint32
{
    Regex_Monostate,
    Regex_Eps,
    Regex_Any,
    Regex_None,
    Regex_Not,
    Regex_Dash,
    Regex_Str,
    Regex_Set,
    Regex_Scope,
};

struct Monostate
{
};

struct State
{
    Option option;
    union {
        Monostate monostate;
        char range[2];
        std::string_view str;
        Node *sequence;
    };

    size_t submit(std::string_view expr, size_t n) const;
};

} // namespace qcc::regex

#endif
