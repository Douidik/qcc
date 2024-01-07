#ifndef QCC_SYNTAX_MAP_HPP
#define QCC_SYNTAX_MAP_HPP

#include "regex.hpp"
#include "token.hpp"
#include <span>

namespace qcc
{

typedef std::span<const std::pair<Token_Type, Regex>> Syntax_Map;
Syntax_Map syntax_map_c89();
Syntax_Map syntax_map_include();
    
} // namespace qcc

#endif
