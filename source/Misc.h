#pragma once
#include "imgui.h"
#include "Math.h"
#include "WinInterop.h"

#include <type_traits>
#include <string>

#define REQUIRE_SEMICOLON enum {}
#define FAIL assert(false)
#define VERIFY(expr) ((expr) ? true : false, FAIL)
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
#define DEBUGLOG(...) DebugPrint(__VA_ARGS__)
#else
#define DEBUGLOG(...) ((void)0)
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

#define _TS_CONCAT(a, b) a ## b
#define TS_CONCAT(a, b) _TS_CONCAT(a, b)

#define Defer auto TS_CONCAT(defer__, __LINE__) = ExitScopeHelp() + [&]()


extern bool g_running;
extern char* g_ClipboardTextData;

template <typename T>
void GenericImGuiTable(const std::string& title, const std::string& fmt, T* firstValue, i32 length = 3)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(title.data());
    for (i32 column = 0; column < length; column++)
    {
        ImGui::TableSetColumnIndex(column + 1);
        std::string string = ToString(fmt.c_str(), firstValue[column]);
        ImGui::TextUnformatted(string.data());
    }
}
