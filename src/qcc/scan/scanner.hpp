#ifndef QCC_SCANNER_HPP
#define QCC_SCANNER_HPP

#include "syntax_map.hpp"
#include "source_snippet.hpp"

namespace qcc
{

struct Scanner
{
    std::string_view source;
    std::string_view next;
    Syntax_Map &map;

    Scanner(std::string_view src, Syntax_Map &map);
    Token tokenize(int128 skip_mask);
    Token dummy_token(Token_Type type) const;

    Error errorf(std::string_view fmt, Token token, auto... args) const
    {
        return Error{"parser error", make_source_snippet(source, token, fmt, args...)};
    }
};

} // namespace qcc

#endif
