#pragma once
#include "WinInterop.h"

#include <type_traits>
#include <string>

#define _V3_CONCAT(a, b) a ## b
#define V3_CONCAT(a, b) _V3_CONCAT(a, b)

#define REQUIRE_SEMICOLON enum {}

#define FAIL assert(false)
#define VERIFY(expr)        [](bool valid) -> bool { assert(valid); return valid; }(!!(expr))
#define VALIDATE(expr)        { if (!VERIFY(expr)) return;     } REQUIRE_SEMICOLON
#define VALIDATE_V(expr, __v) { if (!VERIFY(expr)) return __v; } REQUIRE_SEMICOLON
#define arrsize(arr__) (sizeof(arr__) / sizeof(arr__[0]))

#define ENUMOPS(T) \
constexpr auto operator+(T a)\
{\
    return static_cast<typename std::underlying_type<T>::type>(a);\
}

#define AssertOnce(__cond)                      \
do {                                            \
    static bool did_assert = false;             \
    if (!did_assert && (__cond) == false)       \
    {                                           \
        did_assert = true;                      \
        assert(!#__cond);                       \
    }                                           \
} while(0)


#ifdef _DEBUGPRINT
#define DEBUG_LOG(...) DebugPrint(__VA_ARGS__)
#else
#define DEBUG_LOG(...) ((void)0)
#endif

/**********************************************
 *
 * Defer
 *
 ***************/

template <typename T>
struct ExitScope
{
    T lambda;
    ExitScope(T lambda): lambda(lambda){ }
    ~ExitScope(){ lambda();}
};

struct ExitScopeHelp
{
    template<typename T>
    ExitScope<T> operator+(T t) { return t; }
};

#define Defer auto V3_CONCAT(defer__, __LINE__) = ExitScopeHelp() + [&]()


extern bool g_running;
extern char* g_ClipboardTextData;
