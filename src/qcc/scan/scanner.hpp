#ifndef QCC_SCANNER_HPP
#define QCC_SCANNER_HPP

#include "source_snippet.hpp"
#include "syntax_map.hpp"

namespace qcc
{

struct Scanner
{
    Token tokenize(Source_Context *context, Syntax_Map syntax_map, int128 skip_mask = Token_Mask_Skip);
    Token dummy_token(Token_Type type) const;

    Error errorf(std::string_view fmt, Token token, auto... args) const
    {
        return Error{"scanner error", make_source_snippet(token, fmt, args...)};
    }
};

} // namespace qcc

#endif
