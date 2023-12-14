#include "syntax_map.hpp"

namespace qcc
{

Syntax_Map syntax_map_c89()
{
    static const std::pair<Token_Type, Regex> c89[] = {
        {Token_Blank, "_+"},
        {Token_Comment, "'//' {{{{'\\'^}|^} ~ /'\n'}? /'\n'} | {'/*' ^~ '*/'}"},
        {Token_Directive, "'#' {{{{'\\'^}|^} ~ /'\n'}? /'\n'}"},
        {Token_Id, "{a|'_'} {a|'_'|n}*"},

        {Token_Sizeof, "'sizeof' /!a"},
        {Token_Ampersand, "'&'"},
        {Token_Star, "'*'"},
        {Token_Auto, "'auto' /!a"},
        {Token_Long, "'long' /!a"},
        {Token_Short, "'short' /!a"},
        {Token_Volatile, "'volatile' /!a"},
        {Token_Const, "'const' /!a"},
        {Token_Extern, "'extern' /!a"},
        {Token_Register, "'register' /!a"},
        {Token_Register, "'restrict' /!a"},
        {Token_Static, "'static' /!a"},
        {Token_Signed, "'signed' /!a"},
        {Token_Unsigned, "'unsigned' /!a"},
        {Token_Enum, "'enum' /!a"},
        {Token_Typedef, "'typedef' /!a"},
        {Token_Union, "'union' /!a"},
        {Token_Struct, "'struct' /!a"},
        {Token_Break, "'break' /!a"},
        {Token_Case, "'case' /!a"},
        {Token_Continue, "'continue' /!a"},
        {Token_Default, "'default' /!a"},
        {Token_Do, "'do' /!a"},
        {Token_Else, "'else' /!a"},
        {Token_For, "'for' /!a"},
        {Token_Goto, "'goto' /!a"},
        {Token_If, "'if' /!a"},
        {Token_Return, "'return' /!a"},
        {Token_Switch, "'switch' /!a"},
        {Token_While, "'while' /!a"},

        // TODO! Token float hex
        // {Token_Float_Hex, "{'0x' | '0X'}"
        //                   "{[0-9]+ '.' [0-9]*} |"
        //                   "{[0-9]* '.' [0-9]+}  "
        //                   "{'e'|'E' {'+'|'-'}? [0-9]+}?"
        //                   "a*"},

        {Token_Float, "{[0-9]+ '.' [0-9]*} |"
                      "{[0-9]* '.' [0-9]+}  "
                      "{'e'|'E' {'+'|'-'}? [0-9]+}?"
                      "a"},

        {Token_Int_Bin, "{'0b'|'0B' [0-1]+                     } | a*"},
        {Token_Int_Hex, "{'0x'|'0X' [0-9]|[a-f]|[A-F]+         } | a*"},
        {Token_Int, "    {[0-9]+ {'e'|'E' {'+'|'-'}? [0-9]+ }? } | a*"},

        {Token_String, "'L'? Q {{{'\\'^}|^} ~ /{Q|'\n'}} ? {Q|'\n'}"},
        {Token_Char, "'L'? q {{{'\\'^}|^} ~ /{q|'\n'}} {q|'\n'}"},

        {Token_Dot, "'.'"},
        {Token_Arrow, "'->'"},
        {Token_Comma, "','"},
        {Token_Colon, "':'"},
        {Token_Semicolon, "';'"},

        {Token_Eq, "'=='"},
        {Token_Not_Eq, "'!='"},
        {Token_Less_Eq, "'<='"},
        {Token_Greater_Eq, "'>='"},
        {Token_Less, "'<'"},
        {Token_Greater, "'>'"},

        {Token_Query, "'?'"},
        {Token_Increment, "'++'"},
        {Token_Decrement, "'--'"},
        {Token_Scope_Begin, "'{'"},
        {Token_Scope_End, "'}'"},
        {Token_Paren_Begin, "'('"},
        {Token_Paren_End, "')'"},
        {Token_Crochet_Begin, "'['"},
        {Token_Crochet_End, "']'"},
        {Token_Assign, "'='"},
        {Token_Not, "'!'"},
        {Token_And, "'&&'"},
        {Token_Or, "'||'"},
        {Token_Add, "'+'"},
        {Token_Sub, "'-'"},
        {Token_Div, "'/'"},
        {Token_Mod, "'%'"},

        {Token_Bin_Not, "'~'"},
        {Token_Bin_Or, "'|'"},
        {Token_Bin_Xor, "'^'"},
        {Token_Shift_L, "'<<'"},
        {Token_Shift_R, "'>>'"},

        {Token_None, "^~/_"},
    };

    return {c89};
}

} // namespace qcc
