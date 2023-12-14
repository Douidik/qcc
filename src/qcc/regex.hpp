#ifndef QCC_REGEX_HPP
#define QCC_REGEX_HPP

#include "common.hpp"
#include "regex/match.hpp"
#include "regex/node.hpp"
#include "regex/parser.hpp"
#include <vector>

namespace qcc::regex
{

struct Regex
{
    std::string_view src;
    Node *head;
    Node_Arena arena;

    Regex(std::string_view src) : src(src)
    {
        head = Parser{src, arena}.parse();
    }

    Regex(const char *src) : Regex(std::string_view{src}) {}

    Match match(std::string_view expr) const
    {
        return head != NULL ? Match{expr, head->submit(expr, 0)} : Match{expr, npos};
    }

    Match match(auto begin, auto end) const
    {
        return match(std::string_view{begin, end});
    }
};

inline regex::Regex operator""_rx(const char *src, size_t)
{
    return Regex{src};
}

} // namespace qcc::regex

namespace qcc
{
using regex::Regex;
} // namespace qcc

#endif
