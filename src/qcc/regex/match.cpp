#include "match.hpp"

namespace qcc::regex
{

std::string_view Match::view() const
{
    return std::string_view{begin(), end()};
}

std::string_view Match::next() const
{
    return std::string_view{end(), expr.end()};
}

const char *Match::begin() const
{
    return expr.begin();
}

const char *Match::end() const
{
    if (index != npos) {
        return &expr[index];
    }
    return expr.begin();
}

Match::operator bool() const
{
    return index != npos;
}

} // namespace bee::regex
