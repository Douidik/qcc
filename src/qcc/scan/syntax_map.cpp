#include "syntax_map.hpp"

namespace qcc
{

Syntax_Map syntax_map_c89()
{
    static const std::pair<Token_Type, Regex> c89[] = {
        {Token_Blank, "_+"},
        {Token_Comment, "  {'//' {{{{'\\'^}|^} ~ /'\n'}? /'\n'}}"
                        "| {'/*' ^~                       '*/'}"},
        {Token_None, "'//'|'/*'"},
        {Token_Directive, "'#' {{{{'\\'^}|^} ~ /'\n'}? /'\n'}"},

        {Token_Sizeof, "'sizeof' / !{a|'_'}"},
        {Token_Auto, "'auto' / !{a|'_'}"},
        {Token_Long, "'long' / !{a|'_'}"},
        {Token_Short, "'short' / !{a|'_'}"},
        {Token_Volatile, "'volatile' / !{a|'_'}"},
        {Token_Const, "'const' / !{a|'_'}"},
        {Token_Extern, "'extern' / !{a|'_'}"},
        {Token_Register, "'register' / !{a|'_'}"},
        {Token_Register, "'restrict' / !{a|'_'}"},
        {Token_Static, "'static' / !{a|'_'}"},
        {Token_Signed, "'signed' / !{a|'_'}"},
        {Token_Unsigned, "'unsigned' / !{a|'_'}"},
        {Token_Enum, "'enum' / !{a|'_'}"},
        {Token_Typedef, "'typedef' / !{a|'_'}"},
        {Token_Union, "'union' / !{a|'_'}"},
        {Token_Struct, "'struct' / !{a|'_'}"},
        {Token_Break, "'break' / !{a|'_'}"},
        {Token_Case, "'case' / !{a|'_'}"},
        {Token_Continue, "'continue' / !{a|'_'}"},
        {Token_Default, "'default' / !{a|'_'}"},
        {Token_Do, "'do' / !{a|'_'}"},
        {Token_Else, "'else' / !{a|'_'}"},
        {Token_For, "'for' / !{a|'_'}"},
        {Token_Goto, "'goto' / !{a|'_'}"},
        {Token_If, "'if' / !{a|'_'}"},
        {Token_Return, "'return' / !{a|'_'}"},
        {Token_Switch, "'switch' / !{a|'_'}"},
        {Token_While, "'while' / !{a|'_'}"},
        {Token_Void_Type, "'void' / !{a|'_'}"},
        {Token_Char_Type, "'char' / !{a|'_'}"},
        {Token_Int_Type, "'int'/ !{a|'_'}"},
        {Token_Float_Type, "'float' / !{a|'_'}"},
        {Token_Double_Type, "'double' / !{a|'_'}"},

#define Escape_Sequence                                       \
    "{'\\' {q|Q|'\\'|'a'|'b'|'f'|'n'|'r'|'t'|'v'|'?'|'\n'} |" \
    "      {    [0-8]?[0-8]?[0-8]?                       } |" \
    "      {'x' [0-9]|[a-f]|[A-F]+                       } }"

        {Token_String, "'L'? {QQ} | {Q {{" Escape_Sequence " | {!'\n'}} ~ /{Q|'\n'}} Q}"},
        {Token_Char, "'L'? q " Escape_Sequence " | { !{'\n'|q }} q"},
        {Token_None, "'L'? {Q | {qq?}} {^~ /'\n'}"},
#undef Escape_Sequence

        {Token_Id, "{a|'_'} {a|'_'|n}*"},

        // TODO! Token float hex
        // {Token_Float_Hex, "{'0x' | '0X'}"
        //                   "{[0-9]+ '.' [0-9]*} |"
        //                   "{[0-9]* '.' [0-9]+}  "
        //                   "{'e'|'E' {'+'|'-'}? [0-9]+}?"
        //                   "a*"},

        {Token_Float, "{[0-9]+ '.' [0-9]*} |"
                      "{[0-9]* '.' [0-9]+}  "
                      "{'e'|'E' {'+'|'-'}? [0-9]+}?"
                      "a?"},

        {Token_Int_Bin, "{'0b'|'0B' [0-1]+                     } a*"},
        {Token_Int_Hex, "{'0x'|'0X' [0-9]|[a-f]|[A-F]+         } a*"},
        {Token_Int, "    {[0-9]+ {'e'|'E' {'+'|'-'}? [0-9]+ }? } a*"},

        {Token_Dot, "'.'"},
        {Token_Comma, "','"},
        {Token_Colon, "':'"},
        {Token_Semicolon, "';'"},
        {Token_Shift_L, "'<<'"},
        {Token_Shift_R, "'>>'"},
        {Token_Arrow, "'->'"},

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
        {Token_Add_Assign, "'+='"},
        {Token_Sub_Assign, "'-='"},
        {Token_Mul_Assign, "'*='"},
        {Token_Div_Assign, "'/='"},
        {Token_Mod_Assign, "'%='"},
        {Token_Shift_L_Assign, "'<<='"},
        {Token_Shift_R_Assign, "'<<='"},
        {Token_Bin_And_Assign, "'&='"},
        {Token_Bin_Xor_Assign, "'^='"},
        {Token_Bin_Or_Assign, "'^='"},

        {Token_Add, "'+'"},
        {Token_Sub, "'-'"},
        {Token_Div, "'/'"},
        {Token_Mod, "'%='"},

        {Token_Not, "'!'"},
        {Token_And, "'&&'"},
        {Token_Or, "'||'"},
        {Token_Add, "'+'"},
        {Token_Sub, "'-'"},
        {Token_Div, "'/'"},
        {Token_Mod, "'%'"},
        {Token_Ampersand, "'&'"},
        {Token_Star, "'*'"},

        {Token_Bin_Not, "'~'"},
        {Token_Bin_Or, "'|'"},
        {Token_Bin_Xor, "'^'"},

        {Token_None, "^~/_"},
    };

    return {c89};
}

} // namespace qcc
