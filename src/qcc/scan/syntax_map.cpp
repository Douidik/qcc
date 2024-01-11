#include "syntax_map.hpp"

namespace qcc
{

Syntax_Map syntax_map_c89()
{
    static const std::pair<Token_Type, Regex> c89_map[] = {
	{Token_Newline, "'\n'"},
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

#define Keyword_End "/ !{a|'_'}"
        {Token_Sizeof, "'sizeof'" Keyword_End},
        {Token_Auto, "'auto'" Keyword_End},
        {Token_Long, "'long'" Keyword_End},
        {Token_Short, "'short'" Keyword_End},
        {Token_Volatile, "'volatile'" Keyword_End},
        {Token_Const, "'const'" Keyword_End},
        {Token_Extern, "'extern'" Keyword_End},
        {Token_Register, "'register'" Keyword_End},
        {Token_Register, "'restrict'" Keyword_End},
        {Token_Static, "'static'" Keyword_End},
        {Token_Signed, "'signed'" Keyword_End},
        {Token_Unsigned, "'unsigned'" Keyword_End},
        {Token_Enum, "'enum'" Keyword_End},
        {Token_Typedef, "'typedef'" Keyword_End},
        {Token_Union, "'union'" Keyword_End},
        {Token_Struct, "'struct'" Keyword_End},
        {Token_Break, "'break'" Keyword_End},
        {Token_Case, "'case'" Keyword_End},
        {Token_Continue, "'continue'" Keyword_End},
        {Token_Default, "'default'" Keyword_End},
        {Token_Do, "'do'" Keyword_End},
        {Token_Else, "'else'" Keyword_End},
        {Token_For, "'for'" Keyword_End},
        {Token_Goto, "'goto'" Keyword_End},
        {Token_If, "'if'" Keyword_End},
        {Token_Return, "'return'" Keyword_End},
        {Token_Switch, "'switch'" Keyword_End},
        {Token_While, "'while'" Keyword_End},
        {Token_Void_Type, "'void'" Keyword_End},
        {Token_Char_Type, "'char'" Keyword_End},
        {Token_Int_Type, "'int'" Keyword_End},
        {Token_Float_Type, "'float'" Keyword_End},
        {Token_Double_Type, "'double'" Keyword_End},
#undef Keyword_End

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
        {Token_Add, "'+'"},
        {Token_Sub, "'-'"},
        {Token_Div, "'/'"},
        {Token_Mod, "'%'"},

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
        {Token_Hash_Cwd_Filepath, "Q ^~ Q"},
        {Token_Hash_System_Filepath, "'<' ^~ '>'"},
        {Token_None, "^~/_"},
    };

    return Syntax_Map{include_map};
}

} // namespace qcc
