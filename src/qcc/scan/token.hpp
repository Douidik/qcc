#ifndef QCC_TOKEN_HPP
#define QCC_TOKEN_HPP

#include "common.hpp"

namespace qcc
{

enum Token_Type : int128;

struct Token
{
    std::string_view str;
    Token_Type type;
    bool ok;
    std::string_view type_str;
};

enum Token_Type : int128
{
    Token_None = 0,

    Token_Blank = Bit(int128, 0),
    Token_Eof = Bit(int128, 1),
    Token_Comment = Bit(int128, 2),
    Token_Directive = Bit(int128, 3),
    Token_Id = Bit(int128, 4),

    Token_Sizeof = Bit(int128, 5),
    Token_Ampersand = Bit(int128, 6),
    Token_Star = Bit(int128, 7),
    Token_Auto = Bit(int128, 8),
    Token_Long = Bit(int128, 9),
    Token_Short = Bit(int128, 10),
    Token_Volatile = Bit(int128, 11),
    Token_Const = Bit(int128, 12),
    Token_Extern = Bit(int128, 13),
    Token_Register = Bit(int128, 14),
    Token_Restrict = Bit(int128, 15),
    Token_Static = Bit(int128, 16),
    Token_Signed = Bit(int128, 17),
    Token_Unsigned = Bit(int128, 18),
    Token_Enum = Bit(int128, 19),
    Token_Typedef = Bit(int128, 20),
    Token_Union = Bit(int128, 21),
    Token_Struct = Bit(int128, 22),
    Token_Pointer = Token_Star,
    Token_Break = Bit(int128, 23),
    Token_Case = Bit(int128, 24),
    Token_Continue = Bit(int128, 25),
    Token_Default = Bit(int128, 26),
    Token_Do = Bit(int128, 27),
    Token_Else = Bit(int128, 28),
    Token_For = Bit(int128, 29),
    Token_Goto = Bit(int128, 30),
    Token_If = Bit(int128, 31),
    Token_Return = Bit(int128, 32),
    Token_Switch = Bit(int128, 33),
    Token_While = Bit(int128, 34),

    Token_Float = Bit(int128, 35),
    Token_Float_Hex = Bit(int128, 36),
    Token_Int = Bit(int128, 37),
    Token_Int_Bin = Bit(int128, 38),
    Token_Int_Hex = Bit(int128, 39),
    Token_String = Bit(int128, 40),
    Token_Char = Bit(int128, 41),

    Token_Query = Bit(int128, 42),
    Token_Increment = Bit(int128, 43),
    Token_Decrement = Bit(int128, 44),
    Token_Scope_Begin = Bit(int128, 45),
    Token_Scope_End = Bit(int128, 46),
    Token_Paren_Begin = Bit(int128, 47),
    Token_Paren_End = Bit(int128, 48),
    Token_Crochet_Begin = Bit(int128, 49),
    Token_Crochet_End = Bit(int128, 50),
    Token_Not = Bit(int128, 51),
    Token_And = Bit(int128, 52),
    Token_Or = Bit(int128, 53),
    Token_Add = Bit(int128, 54),
    Token_Sub = Bit(int128, 55),
    Token_Mul = Token_Star,
    Token_Div = Bit(int128, 56),
    Token_Mod = Bit(int128, 57),

    Token_Assign = Bit(int128, 58),
    Token_Add_Assign = Bit(int128, 59),
    Token_Sub_Assign = Bit(int128, 60),
    Token_Mul_Assign = Bit(int128, 61),
    Token_Div_Assign = Bit(int128, 62),
    Token_Mod_Assign = Bit(int128, 63),
    Token_Shift_L_Assign = Bit(int128, 64),
    Token_Shift_R_Assign = Bit(int128, 65),
    Token_Bin_And_Assign = Bit(int128, 66),
    Token_Bin_Xor_Assign = Bit(int128, 67),
    Token_Bin_Or_Assign = Bit(int128, 68),

    Token_Bin_Not = Bit(int128, 69),
    Token_Bin_And = Token_Ampersand,
    Token_Bin_Or = Bit(int128, 70),
    Token_Bin_Xor = Bit(int128, 71),
    Token_Shift_L = Bit(int128, 72),
    Token_Shift_R = Bit(int128, 73),

    Token_Eq = Bit(int128, 74),
    Token_Not_Eq = Bit(int128, 75),
    Token_Less = Bit(int128, 76),
    Token_Greater = Bit(int128, 78),
    Token_Less_Eq = Bit(int128, 79),
    Token_Greater_Eq = Bit(int128, 80),

    Token_Deref = Token_Star,
    Token_Address = Token_Ampersand,
    Token_Dot = Bit(int128, 81),
    Token_Arrow = Bit(int128, 82),
    Token_Comma = Bit(int128, 83),
    Token_Colon = Bit(int128, 84),
    Token_Semicolon = Bit(int128, 85),

    Token_Void_Type = Bit(int128, 86),
    Token_Char_Type = Bit(int128, 87),
    Token_Int_Type = Bit(int128, 88),
    Token_Float_Type = Bit(int128, 89),
    Token_Double_Type = Bit(int128, 90),

    Token_Merged = Bit(int128, 91),
    Token_Type_End = Bit(int128, 92),
};

const int128 Token_Mask_Expression =
    Token_Id | Token_Char | Token_String | Token_Int | Token_Int_Bin | Token_Int_Hex | Token_Float |
    Token_Increment | Token_Decrement | Token_Add | Token_Sub | Token_Mul | Token_Div | Token_Mod |
    Token_Not | Token_Bin_Not | Token_Bin_And | Token_Bin_Or | Token_Bin_Xor | Token_Shift_L | Token_Shift_R |
    Token_Eq | Token_Not_Eq | Token_Less | Token_Less_Eq | Token_Greater | Token_Greater_Eq |
    Token_Paren_Begin | Token_Assign | Token_Dot | Token_Arrow | Token_Deref;

const int128 Token_Mask_Statement = Token_Scope_Begin | Token_If | Token_While | Token_For;

const int128 Token_Mask_Each = ~((int128)0);

const int128 Token_Mask_Type = Token_Auto | Token_Long | Token_Short | Token_Volatile | Token_Const |
                               Token_Extern | Token_Register | Token_Static | Token_Signed | Token_Unsigned |
                               Token_Int_Type | Token_Char_Type | Token_Float_Type | Token_Double_Type |
                               Token_Void_Type | Token_Struct | Token_Union | Token_Enum;

const int128 Token_Mask_Type_Cvr = Token_Const | Token_Volatile | Token_Restrict;
const int128 Token_Mask_Type_Storage = Token_Extern | Token_Register | Token_Static | Token_Auto;
const int128 Token_Mask_Fundamental =
    Token_Int_Type | Token_Char_Type | Token_Float_Type | Token_Double_Type | Token_Void_Type;

const int128 Token_Mask_Record = Token_Struct | Token_Union | Token_Enum;

const int128 Token_Mask_Binary_Assign = Token_Assign | Token_Add_Assign | Token_Sub_Assign |
                                        Token_Mul_Assign | Token_Div_Assign | Token_Mod_Assign |
                                        Token_Shift_L_Assign | Token_Shift_R_Assign | Token_Bin_And_Assign |
                                        Token_Bin_Xor_Assign | Token_Bin_Or_Assign;

const int128 Token_Mask_Shift = Token_Shift_L | Token_Shift_R | Token_Shift_L_Assign | Token_Shift_R_Assign;

const int128 Token_Mask_Bin = Token_Shift_L_Assign | Token_Shift_R_Assign | Token_Bin_And_Assign |
                              Token_Bin_Xor_Assign | Token_Bin_Or_Assign | Token_Bin_Not | Token_Bin_And |
                              Token_Bin_Or | Token_Bin_Xor | Token_Shift_L | Token_Shift_R;

static Token operator|(Token lhs, Token rhs)
{
    if (!lhs.type)
        return rhs;
    if (!rhs.type)
        return lhs;

    Token token = {};

    const char *begin = lhs.str.begin();
    const char *end = rhs.str.end();

    token.str = std::string_view{};

    return Token{
        std::string_view{std::min(begin, end), std::max(begin, end)},
        Token_Merged,
        false,
    };
}

static Token operator|=(Token &lhs, Token &rhs)
{
    return lhs = (lhs | rhs);
}

constexpr std::string_view token_type_str(Token_Type type)
{
    switch (type) {
    case Token_None:
        return "none";
    case Token_Blank:
        return "blank";
    case Token_Eof:
        return "end-of-file";
    case Token_Comment:
        return "comment";
    case Token_Directive:
        return "directive";
    case Token_Id:
        return "identifier";
    case Token_Sizeof:
        return "sizeof";
    case Token_Ampersand:
        return "&";
    case Token_Star:
        return "*";
    case Token_Auto:
        return "auto";
    case Token_Long:
        return "long";
    case Token_Short:
        return "short";
    case Token_Volatile:
        return "volatile";
    case Token_Const:
        return "const";
    case Token_Extern:
        return "extern";
    case Token_Register:
        return "register";
    case Token_Restrict:
        return "restrict";
    case Token_Static:
        return "static";
    case Token_Signed:
        return "signed";
    case Token_Unsigned:
        return "unsigned";
    case Token_Enum:
        return "enum";
    case Token_Typedef:
        return "typedef";
    case Token_Union:
        return "union";
    case Token_Struct:
        return "struct";
    case Token_Break:
        return "break";
    case Token_Case:
        return "case";
    case Token_Continue:
        return "continue";
    case Token_Default:
        return "default";
    case Token_Do:
        return "do";
    case Token_Else:
        return "else";
    case Token_For:
        return "for";
    case Token_Goto:
        return "goto";
    case Token_If:
        return "if";
    case Token_Return:
        return "return";
    case Token_Switch:
        return "switch";
    case Token_While:
        return "while";
    case Token_Float:
        return "float-literal";
    case Token_Float_Hex:
        return "hex-float-literal";
    case Token_Int:
        return "int-literal";
    case Token_Int_Bin:
        return "binary-int-literal";
    case Token_Int_Hex:
        return "hex-int-literal";
    case Token_String:
        return "string-literal";
    case Token_Char:
        return "char-literal";
    case Token_Query:
        return "?";
    case Token_Increment:
        return "++";
    case Token_Decrement:
        return "--";
    case Token_Scope_Begin:
        return "{";
    case Token_Scope_End:
        return "}";
    case Token_Paren_Begin:
        return "(";
    case Token_Paren_End:
        return ")";
    case Token_Crochet_Begin:
        return "[";
    case Token_Crochet_End:
        return "]";
    case Token_Assign:
        return "=";
    case Token_Not:
        return "!";
    case Token_And:
        return "&&";
    case Token_Or:
        return "||";
    case Token_Add:
        return "+";
    case Token_Sub:
        return "-";
    case Token_Div:
        return "/";
    case Token_Mod:
        return "%";
    case Token_Bin_Not:
        return "~";
    case Token_Bin_Or:
        return "|";
    case Token_Bin_Xor:
        return "^";
    case Token_Shift_L:
        return "<<";
    case Token_Shift_R:
        return ">>";
    case Token_Eq:
        return "==";
    case Token_Not_Eq:
        return "!=";
    case Token_Less:
        return "<";
    case Token_Greater:
        return ">";
    case Token_Less_Eq:
        return "<=";
    case Token_Greater_Eq:
        return ">=";
    case Token_Dot:
        return ".";
    case Token_Arrow:
        return "->";
    case Token_Comma:
        return ",";
    case Token_Colon:
        return ",";
    case Token_Semicolon:
        return ";";
    case Token_Void_Type:
        return "void";
    case Token_Char_Type:
        return "char";
    case Token_Int_Type:
        return "int";
    case Token_Float_Type:
        return "float";
    case Token_Double_Type:
        return "double";
    default:
        return "?";
    }
}

} // namespace qcc

#endif
