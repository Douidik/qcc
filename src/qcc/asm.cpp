#include "asm.hpp"
#include "ast.hpp"

namespace qcc
{

Asm::Asm(Ast &ast, std::string_view source, std::ostream &stream) :
    ast(ast), stream(stream), type_system(ast.type_system), source(source)
{
}

Label Asm::make_label(Label_Type type)
{
    return Label{type, label_count++};
}

} // namespace qcc
