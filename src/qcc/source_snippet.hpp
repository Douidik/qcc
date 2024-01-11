#ifndef QCC_SOURCE_SNIPPET_HPP
#define QCC_SOURCE_SNIPPET_HPP

#include "scan/token.hpp"
#include <algorithm>

namespace qcc
{

// Todo! snippet for multiple line tokens
std::string make_source_snippet(Token token, std::string_view fmt, auto... args)
{
    auto digits_count = [](int64 number) -> size_t {
        size_t digits = 1;
        for (; number > 9; number /= 10)
            digits++;
        return digits;
    };

    Source_Context context = token.context;
    std::string_view source = context.source;
    std::string_view str = token.str;
    std::string snippet = "";

    auto token_rbegin = std::find(str.rend(), source.rend(), '\n');
    auto line_begin = Max(token_rbegin.base(), source.begin());
    auto line_end = std::find(str.end(), source.end(), '\n');
    std::string_view line = {line_begin, line_end};
    uint32 cursor = str.begin() - line_begin + digits_count(context.line);

#define S std::back_inserter(snippet)
    fmt::format_to(S, "with ({}:{}) {{\n", context.filepath->str, context.line);
    fmt::format_to(S, " {} | {}\n", context.line, line);
    fmt::format_to(S, "    {:>{}}{:^>{}} ", "", cursor, "^", str.size());
    fmt::format_to(S, fmt::runtime(fmt), args...);
    fmt::format_to(S, "\n}}");
#undef S
    return snippet;
}

} // namespace qcc

#endif
