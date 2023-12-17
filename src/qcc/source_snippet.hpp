#ifndef SOURCE_SNIPPET_HPP
#define SOURCE_SNIPPET_HPP

#include "scan/token.hpp"
#include <algorithm>

namespace qcc
{

std::string make_source_snippet(std::string_view source, Token token, std::string_view fmt = "", auto... args)
{
    auto digits_count = [](int64 number) -> size_t {
        size_t digits = 1;
        for (; number > 9; number /= 10)
            digits++;
        return digits;
    };

    uint32 line_number = std::count(source.begin(), token.str.begin(), '\n');
    auto rbegin = std::find(token.str.rend(), source.rend(), '\n');
    auto begin = std::max(rbegin.base(), source.begin());
    auto end = std::find(token.str.end(), source.end(), '\n');
    std::string desc = fmt::format(fmt::runtime(fmt), args...);
    uint32 cursor = token.str.begin() - begin + digits_count(line_number);

    return fmt::format(R"(with {{
  {} | {}
     {:>{}}{:^>{}} {}
}})",
                       line_number, std::string_view{begin, end}, "", cursor, "^", token.str.size(), desc);
}

} // namespace qcc

#endif
