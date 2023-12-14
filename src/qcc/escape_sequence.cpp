#include "escape_sequence.hpp"

namespace qcc {

std::string escape_string(std::string_view s, int n)
{
    if (n < 1) {
        return std::string{s};
    }

    std::string esc{};
    esc.reserve(s.size());

    for (const char &c : s) {
        esc.append(escape_sequence(c));
    }

    return escape_string(esc, n - 1);
}

std::string un_escape_string(std::string_view s, int n)
{
    if (n < 1) {
        return std::string{s};
    }

    std::string esc{};
    esc.reserve(s.size());

    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] != '\\') {
            esc.push_back(s[i]);
        } else {
            i++;
            if (s.size() <= i)
                throw Error{"un_escape_string() error", "unterminated escape sequence"};
            if (s[i] != '\\')
                esc.push_back(un_escape_sequence(s[i]));
        }
    }

    return un_escape_string(esc, n - 1);
}

} // namespace qcc
