#ifndef QCC_FWD_HPP
#define QCC_FWD_HPP

#include "common.hpp"

namespace qcc
{

struct Ast;
struct Type_System;
struct Type;

enum Statement_Kind : uint32;
enum Define_Type : uint32;
struct Statement;
struct Scope_Statement;
struct Function_Statement;
struct Define_Statement;
struct Expression_Statement;
struct Condition_Statement;
struct While_Statement;
struct For_Statement;
struct Jump_Statement;
struct Record_Statement;
struct Return_Statement;

enum Expression_Kind : uint32;
enum Expression_Order : uint32;
struct Expression;
struct Unary_Expression;
struct Binary_Expression;
struct Argument_Expression;
struct Invoke_Expression;
struct Comma_Expression;
struct String_Expression;
struct Int_Expression;
struct Float_Expression;
struct Id_Expression;
struct Nested_Expression;
struct Assign_Expression;
struct Cast_Expression;
struct Dot_Expression;

enum Object_Kind : uint32;
enum Type_Mod : uint32;
enum Type_Storage : uint32;
struct Object;
struct Function;
struct Variable;
struct Typedef;
struct Record;

} // namespace qcc

#endif
