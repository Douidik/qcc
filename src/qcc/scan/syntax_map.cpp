#include "syntax_map.hpp"

namespace qcc
{

Syntax_Map syntax_map_c89()
{
    static const std::pair<Token_Type, Regex> c89_map[] = {
        {Token_Blank, "_+"},
        {Token_Comment, "  {'//' {{{{'\\'^}|^} ~ /'\n'}? /'\n'}}"
                        "| {'/*' ^~                       '*/'}"},
        {Token_None, "'//'|'/*'"},

#define Hash "'#' ^~ "
        {Token_Hash_Include, Hash "'include'"},
        {Token_Hash_Define, Hash "'define'"},
        {Token_Hash_Undef, Hash "'undef'"},
        {Token_Hash_Ifdef, Hash "'ifdef'"},
        {Token_Hash_Ifndef, Hash "'ifndef'"},
        {Token_Hash_Elif, Hash "'elif'"},
        {Token_Hash_Else, Hash "'else'"},
        {Token_Hash_Endif, Hash "'endif'"},
#undef Hash

#define No_Ids "/ !{a|'_'}"
        {Token_Sizeof, "'sizeof'" No_Ids},
        {Token_Auto, "'auto'" No_Ids},
        {Token_Long, "'long'" No_Ids},
        {Token_Short, "'short'" No_Ids},
        {Token_Volatile, "'volatile'" No_Ids},
        {Token_Const, "'const'" No_Ids},
        {Token_Extern, "'extern'" No_Ids},
        {Token_Register, "'register'" No_Ids},
        {Token_Register, "'restrict'" No_Ids},
        {Token_Static, "'static'" No_Ids},
        {Token_Signed, "'signed'" No_Ids},
        {Token_Unsigned, "'unsigned'" No_Ids},
        {Token_Enum, "'enum'" No_Ids},
        {Token_Typedef, "'typedef'" No_Ids},
        {Token_Union, "'union'" No_Ids},
        {Token_Struct, "'struct'" No_Ids},
        {Token_Break, "'break'" No_Ids},
        {Token_Case, "'case'" No_Ids},
        {Token_Continue, "'continue'" No_Ids},
        {Token_Default, "'default'" No_Ids},
        {Token_Do, "'do'" No_Ids},
        {Token_Else, "'else'" No_Ids},
        {Token_For, "'for'" No_Ids},
        {Token_Goto, "'goto'" No_Ids},
        {Token_If, "'if'" No_Ids},
        {Token_Return, "'return'" No_Ids},
        {Token_Switch, "'switch'" No_Ids},
        {Token_While, "'while'" No_Ids},
        {Token_Void_Type, "'void'" No_Ids},
        {Token_Char_Type, "'char'" No_Ids},
        {Token_Int_Type, "'int'" No_Ids},
        {Token_Float_Type, "'float'" No_Ids},
        {Token_Double_Type, "'double'" No_Ids},
#undef No_Ids

#define Escape_Sequence                                       \
    "{'\\' {q|Q|'\\'|'a'|'b'|'f'|'n'|'r'|'t'|'v'|'?'|'\n'} |" \
    "      {    [0-8]?[0-8]?[0-8]?                       } |" \
    "      {'x' [0-9]|[a-f]|[A-F]+                       } }"

        {Token_String, "'L'? {QQ} | {Q {{" Escape_Sequence " | {!'\n'}} ~ /{Q|'\n'}} Q}"},
        {Token_Char, "'L'? q " Escape_Sequence " | { !{'\n'|q }} q"},
        {Token_None, "'L'? {Q | {qq?}} {^~ /'\n'}"},
#undef Escape_Sequence

        {Token_Id, "{a|'_'} {a|'_'|n}*"},

        // Todo! Token float hex
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

        {Token_Bitwise_Not, "'~'"},
        {Token_Bitwise_Or, "'|'"},
        {Token_Bitwise_Xor, "'^'"},

        {Token_None, "^~/_"},
    };

    return Syntax_Map{c89_map};
}

Syntax_Map syntax_map_include()
{
    static const std::pair<Token_Type, Regex> include_map[] = {
        {Token_Blank, "_+"},
        {Token_Hash_Project_Filepath, "Q ^~ Q"},
        {Token_Hash_System_Filepath, "'<' ^~ '>'"},
        {Token_None, "^~/_"},
    };

    return Syntax_Map{include_map};
}

} // namespace qcc
