#ifndef QCC_ARENA_HPP
#define QCC_ARENA_HPP

#include "common.hpp"
#include <deque>
#include <fmt/printf.h>

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

// template <typename T, size_t N>
// struct Arena
// {
//     T data[N];
//     size_t size;
//     Arena<T, N> *next;

//     Arena(T value = T{})
//         : size(0),
//           data(value)
//     {
//     }

//     Arena(auto begin, auto end)
//     {
//         size_t range_size = std::distance(begin, end);
//         size = std::min(range_size, N);
//         std::move(begin, &begin[size], data);

//         if (range_size > N) {
//             next = new Arena{&begin[N], end};
//         }
//     }

//     ~Arena()
//     {
//         if (next != NULL)
//             delete next;
//     }

//     T *push(const T &&value)
//     {
//         Arena<T, N> *push_arena = NULL;
//         for (Arena<T, N> *arena = this; arena != NULL; arena = arena->next) {
//             if (!arena->next) {
//                 arena->next = new Arena{};
//             }
//             if (arena->size < N) {
//                 push_arena = arena;
//                 break;
//             }
//         }

//         // return &(push_->data[chunk->size++] = value);
//     }

//     T *pop()
//     {
//         Arena<T, N> *arena = this;
//         for (; arena->size > 0 and arena->next != NULL; arena = arena->next)
//         {
//         }
//     }

//     Arena<T, N> *chunks_end()
//     {
//         Arena<T, N> *arena = this;
//         for (; arena->size > 0 and arena->next != NULL; arena = arena->next)
//         {
//         }
//         return arena;
//     }

//     size_t len() const
//     {
//         size_t len = 0;
//         for (Arena<T, N> *arena = this; arena != NULL; arena = arena->next) {
//             len += arena->size;
//         }
//         return len;
//     }

//     Error errorf(std::string_view s, auto... args)
//     {
//         return Error{"arena error", fmt::format(s, args...)};
//     }
// };

// template <typename T, size_t N>
// struct Arena_Chunk
// {
//     T data[N];
// };

// template <typename T, size_t N>
// struct Arena
// {
//     std::deque<Arena_Chunk<T, N>> chunks;
//     size_t len;

//     Arena()
//         : len(0)
//     {
//     }

//     Arena(auto begin, auto end)
//         : len(end - begin)
//     {
//         for (size_t i = 0; i < len; i++) {
//             Arena_Chunk<T, N> *chunk = &chunks.emplace_back({});
//             std::move(begin, end, chunk->data);
//         }
//     }

//     T *at(size_t i)
//     {
//         if (i < len) {
//             return NULL;
//         }

//         Arena_Chunk<T, N> *chunk = &chunks.at(i / N);
//         return &(chunk->data[i % N]);
//     }

//     T *push(const T &&value)
//     {
//         while ((len + 1) > (chunks.size() * N)) {
//             chunks.emplace_back({});
//         }

//         std::array<T, N> &chunk = chunks.back();
//         return &(chunk[len++ % N] = value);
//     }

//     T *pop()
//     {
//         return len > 0 ? at(--len) : NULL;
//     }

//     T *begin()
//     {
//         return at(0);
//     }

//     T *end()
//     {
//         return at(len - 1);
//     }
// };

// static int test()
// {
//     int src[3] = {1, 2, 3};

//     Arena<int, 16> ints{src, src + 3};
//     ints.push(1);
//     return *ints.begin();
// }

} // namespace qcc

#endif
