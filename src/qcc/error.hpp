#ifndef QCC_ERROR_H
#define QCC_ERROR_H

#include <exception>
#include <string>
#include <string_view>

namespace qcc {

struct Error : std::exception
{
    std::string_view name;
    std::string buffer;

    Error(std::string_view name, std::string buffer)
        : name{name},
          buffer{buffer}
    {
    }

    const char *what() const noexcept override
    {
        return buffer.c_str();
    }
};

} // namespace qcc

#endif
