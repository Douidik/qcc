#include "statement.hpp"
#include "object.hpp"

namespace qcc
{

Object *Scope_Statement::object(std::string_view name)
{
    if (objects.contains(name))
        return objects.at(name);
    if (owner != NULL)
        return owner->object(name);
    return NULL;
}

Record *Scope_Statement::record(Type_Kind kind, std::string_view name)
{
    if (records.contains(name)) {
        if (records[name]->type.kind != kind)
            return NULL;
        return records[name];
    }
    if (owner != NULL)
        return owner->record(kind, name);
    return NULL;
}

} // namespace qcc
