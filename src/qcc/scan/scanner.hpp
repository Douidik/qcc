#ifndef QCC_SCANNER_HPP
#define QCC_SCANNER_HPP

#include "syntax_map.hpp"

namespace qcc
{

struct Scanner
{
    std::string_view src;
    std::string_view next;
    Syntax_Map &map;
    
    Scanner(std::string_view src, Syntax_Map &map);
    Token tokenize();
    Token dummy_token(Token_Type type) const;
};

} // namespace qcc

#endif
