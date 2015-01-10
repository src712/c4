#ifndef UTIL_H
#define UTIL_H

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <memory>

template<typename T> inline T* nonNull(T* const p)
{
	assert(p);
	return p;
}

// C++14
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

[[noreturn]] static inline void panic(char const* const file, int const line, char const* const msg)
{
	fprintf(stderr, "%s:%d: %s\n", file, line, msg);
	abort();
}

#define PANIC(msg) panic(__FILE__, __LINE__, (msg))

static inline bool strEq(char const* const a, char const* const b)
{
	return strcmp(a, b) == 0;
}

#endif
