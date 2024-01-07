#ifndef QCC_SCANNER_HPP
#define QCC_SCANNER_HPP

#include "source_snippet.hpp"
#include "syntax_map.hpp"
#include <stack>
#include <unordered_map>

namespace qcc
{

struct Scanner_Source
{
    std::string_view data;
    std::string_view stream;
};

struct Scanner
{
    Scanner_Source file;
    fs::path cwd;

    std::stack<Scanner_Source> macro_stack;
    std::vector<std::string> includes;
    std::unordered_map<std::string_view, Scanner_Source> definition_map;

    Scanner(std::string_view file, fs::path cwd);
    Token tokenize(Syntax_Map syntax_map, int128 skip_mask = Token_Mask_Skip);
    Token dummy_token(Token_Type type) const;
    Scanner_Source &current_source();

    Token parse_hash_token(Token token);

    Error errorf(std::string_view fmt, Token token, auto... args) const
    {
        return Error{"parser error", make_source_snippet(token, fmt, args...)};
    }
};

} // namespace qcc

#endif
