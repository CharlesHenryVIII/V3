#include "WinInterop.h"
#include "Misc.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shlobj_core.h>
#include <string>
#include <thread>

bool CreateFolder(const std::string& folderLocation)
{
    BOOL result = CreateDirectoryA(folderLocation.c_str(), NULL);
    return !(result == 0);
}

void DebugPrint(const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, list);
    OutputDebugStringA(buffer);
    va_end(list);
}

std::string ToString(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buffer[4096];
    i32 i = vsnprintf(buffer, arrsize(buffer), fmt, args);
    va_end(args);
    return buffer;
}

//
// https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code?view=vs-2017
// Usage: SetThreadName ((DWORD)-1, "MainThread");
//
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
 } THREADNAME_INFO;
#pragma pack(pop)
void SetThreadName(DWORD dwThreadID, const char* threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try{
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER){
    }
#pragma warning(pop)
}

void SetThreadName(std::thread::native_handle_type threadID, std::string name)
{
    SetThreadName(GetThreadId(threadID), name.c_str());
}

void Sleep_Thread(i64 milliseconds)
{
    Sleep(1000);
}
