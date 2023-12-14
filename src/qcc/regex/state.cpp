#include "state.hpp"
#include "node.hpp"

namespace qcc::regex {

size_t State::submit(std::string_view expr, size_t n) const
{
    if (option != Regex_Eps and n >= expr.size())
        return npos;

    switch (option) {
    case Regex_Eps:
        return n;

    case Regex_Any:
        return n + 1;

    case Regex_None:
        return npos;

    case Regex_Not:
        return sequence->submit(expr, n) != npos ? npos : n + 1;

    case Regex_Dash:
        return sequence->submit(expr, n) != npos ? n : npos;

    case Regex_Str: {
        std::string_view cmp = expr.substr(n, str.size());
        return cmp == str ? n + str.size() : npos;
    }

    case Regex_Set:
        return str.find(expr[n]) != npos ? n + 1 : npos;

    case Regex_Scope:
        return range[0] <= expr[n] and expr[n] <= range[1] ? n + 1 : npos;

    default:
        return npos;
    }
}

} // namespace qcc::regex
