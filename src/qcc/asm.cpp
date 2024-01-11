#include "asm.hpp"
#include "ast.hpp"

namespace qcc
{

Asm::Asm(Ast &ast, Allocator &allocator, std::ostream &stream) :
    ast(ast), stream(stream), allocator(allocator), label_count(0)
{
}

Label Asm::emit_label(Label_Type type)
{
    return Label{type, label_count++};
}

} // namespace qcc
