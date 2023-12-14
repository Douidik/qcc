#ifndef QCC_PARSER_HPP
#define QCC_PARSER_HPP

#include "fwd.hpp"
#include "scan/scanner.hpp"
#include "type_system.hpp"
#include <algorithm>

namespace qcc
{

/*
  stolen from: https://github.com/joehattori/tgocc/blob/master/parser/parse.go
        program    = (function | globalVar)*
    function   = baseType types.TypeDecl "(" (ident ("," ident)* )? ")" ("{" stmt* "}" | ";")
    globalVar  = decl
    stmt       = expr ";"
                | "{" stmt* "}"
                | "return" expr ";"
                | "if" "(" expr ")" stmt ("else" stmt) ?
                | "while" "(" expr ")" stmt
                | "do" stmt "while" "(" expr ")" ";"
                | "for" "(" (expr? ";" | decl) expr? ";" expr? ")" stmt
                | "typedef" types.Type ident ("[" constExpr "]")* ";"
                | "switch" "(" expr ")" "{" switchCase* ("default" ":" stmt*)? }"
                | decl
    switchCase = "case" num ":" stmt*
    decl       = baseType types.TypeDecl ("[" constExpr "]")* "=" initialize ;" | baseTy ";"
    tyDecl     = "*"* (ident | "(" types.TypeDecl ")")
    expr       = assign
    assign     = ternary (("=" | "+=" | "-=" | "*=" | "/=") assign) ?
    ternary    = logOr ("?" expr ":" ternary)?
    logOr      = logAnd ("||" logAnd)*
    logAnd     = bitOr ("&&" bitOr)*
    bitOr      = bitXor ("|" bitXor)*
    bitXor     = bitAnd ("^" bitAnd)*
    bitAnd     = equality ("&" equality)*
    equality   = relational ("==" relational | "!=" relational)*
    relational = shift ("<" shift | "<=" shift | ">" shift | ">=" shift)*
    shift      = add ("<<" add | ">>" add | "<<=" add | ">>=" add)
    add        = mul ("+" mul | "-" mul)*
    mul        = cat ("*" cast | "/" cast)*
    cast       = "(" baseType "*"*  ")" cast | unary
    unary      = ("+" | "-" | "*" | "&" | "!" | "~")? cast | ("++" | "--") unary | postfix
    postfix    = primary (("[" expr "]") | ("." ident) | ("->" ident) | "++" | "--")*
    primary    =  num
                | "sizeof" unary
                | str
                | ident ("(" (expr ("," expr)* )? ")")?
                | "(" expr ")"
                | stmtExpr
    stmtExpr   = "(" "{" stmt+ "}" ")"
*/

enum Parse_Define_Type : uint32
{
    Parse_Define_Struct = Bit(uint32, 1),
    Parse_Define_Union = Bit(uint32, 2),
    Parse_Define_Enum = Bit(uint32, 3),
    Parse_Define_Variable = Bit(uint32, 4),
};

struct Parser
{
    Ast &ast;
    Scanner &scanner;
    Type_System &type_system;
    std::deque<Token> token_queue;
    std::deque<Scope_Statement *> stack;

    Parser(Ast &ast, Scanner &scanner);

    Statement *parse();
    Statement *parse_statement();
    Type parse_type();

    Scope_Statement *parse_scope_statement(int128 end_mask);
    Statement *parse_define_or_function_statement();
    Define_Statement *parse_define_statement(Type type, Token name, Parse_Define_Type define_type,
                                             Define_Statement *previous);
    uint64 parse_enum_value(Define_Statement *define_statement, Define_Statement *previous);
    Function_Statement *parse_function_statement(Type return_type, Token name);

    Record_Statement *parse_record_statement(Token keyword);
    Record_Statement *parse_struct_statement(Token keyword);
    Scope_Statement *parse_record_scope_statement(Token keyword);
    Record_Statement *parse_enum_statement();

    Expression *parse_boolean_expression();
    Statement *parse_scope_or_expression_statement();
    Condition_Statement *parse_condition_statement();
    While_Statement *parse_while_statement();
    For_Statement *parse_for_statement();

    Expression_Statement *parse_expression_statement();
    Expression *parse_expression(Expression *previous);
    Comma_Expression *parse_comma_expression(Token token, Expression *lhs);
    Unary_Expression *parse_unary_expression(Token operation, Expression_Order order, Expression *operand);
    Binary_Expression *parse_binary_expression(Token operation, Expression *lhs, Expression *rhs);
    Id_Expression *parse_id_expression(Token token);
    String_Expression *parse_string_expression(Token token);
    Int_Expression *parse_int_expression(Token token);
    uint32 parse_int_flags(Token token, std::string_view suffix);
    Float_Expression *parse_float_expression(Token token);

    int64 parse_constant(Token token, Expression *expression);

    Scope_Statement *scope_in();
    bool is_eof() const;
    bool peek_until(int128 mask);
    Token peek(int128 mask);
    Token scan(int128 mask);
    Token expect(int128 mask, std::string_view context);

    Error errorf(std::string_view fmt, Token token, auto... args) const
    {
        auto int_digits = [](int64 n) -> size_t {
            size_t digits = 1;
            for (; n > 9; n /= 10) {
                digits++;
            }
            return digits;
        };

        // Generate code snippet
        std::string_view src = scanner.src;
        uint32 line_number = std::count(src.begin(), token.str.begin(), '\n');
        auto rbegin = std::find(token.str.rend(), src.rend(), '\n');
        auto begin = std::max(rbegin.base(), src.begin());
        auto end = std::find(token.str.end(), src.end(), '\n');
        std::string desc = fmt::format(fmt::runtime(fmt), args...);
        uint32 cursor = token.str.begin() - begin + int_digits(line_number);

        return Error{
            "parser error",
            fmt::format(R"(with {{
  {} | {}
      {:>{}}{:^>{}} {}
}})",
                        line_number, std::string_view{begin, end}, "", cursor, "^", token.str.size(), desc),
        };
    }
};

} // namespace qcc

#endif
