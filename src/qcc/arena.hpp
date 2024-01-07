#ifndef QCC_ARENA_HPP
#define QCC_ARENA_HPP

#include "common.hpp"

namespace qcc
{

template <typename T, size_t N>
struct Arena
{
    T data[N];
    size_t size;

    Arena(T value = {}) : size(0)
    {
        std::fill_n(data, N, value);
    }

    Arena(auto begin, auto end) : size(end - begin)
    {
        size_t range_size = std::distance(begin, end);
        std::move(begin, &begin[size], data);
    }

    T *push(const T &&value)
    {
        if (size + 1 > N)
            throw errorf("cannot push() capacity exceeded");
        return &(data[size++] = value);
    }

    T *emplace(auto &&...args)
    {
        return push(T{args...});
    }

    T *pop()
    {
        if (size <= 0)
            throw errorf("cannot pop() on empty container");
        return &(data[--size]);
    }

    T *begin()
    {
        return size != 0 ? &data[0] : NULL;
    }

    T *end()
    {
        return size != 0 ? &data[size - 1] : NULL;
    }

    Error errorf(std::string_view fmt, auto... args)
    {
        return Error{"arena error", fmt::format(fmt::runtime(fmt), args...)};
    }
};

} // namespace qcc

#endif
