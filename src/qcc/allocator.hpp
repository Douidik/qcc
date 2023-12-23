#ifndef QCC_ALLOCATOR_HPP
#define QCC_ALLOCATOR_HPP

#include "fwd.hpp"
#include <set>
#include <unordered_map>
#include <vector>

namespace qcc
{

struct Use_Range
{
    uint32 begin, end;
};

struct Allocator
{
    Ast &ast;
    uint32 gpr_count;
    uint32 fpr_count;
    std::unordered_map<Variable *, Use_Range> uses_range;
    std::vector<std::set<Variable *>> uses_timeline;

    Allocator(Ast &ast, uint32 gpr_count, uint32 fpr_count);
    void allocate();

    int64 create_function_stack_push(Variable *variable, int64 offset, int64 alignment);
    void create_function_stack(Function_Statement *function_statement);
    void parse_begin_of_use(Variable *variable);
    void parse_end_of_use(Variable *variable);

    void parse_new_use(Variable *variable);
    void parse_statement_use_ranges(Statement *statement);
    void parse_expression_use_ranges(Expression *expression);
};

} // namespace qcc

#endif
