#include "WinInterop.h"
#include "Debug.h"
#include "Math.h"

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
void DebugPrint(const wchar_t* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    wchar_t buffer[4096];
    _vsnwprintf(buffer, sizeof(buffer), fmt, list);
    OutputDebugStringW(buffer);
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

void ScanDirectoryForFileNames(const std::string& dir, std::vector<std::string>& out)
{
    out.clear();

    std::string d = dir;
    if (dir.size() < 2)
    {
        d = "*";
    }
    else
    {

        if (d[d.size() - 1] != '*')
        {
            if (d[d.size() - 1] != '/')
            {
                d += '/';
            }
            d += '*';
        }
    }
	

    WIN32_FIND_DATAA find_data;
    HANDLE handle = FindFirstFileA(d.c_str(), &find_data);
    while (true)
	{
		if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
            out.push_back(find_data.cFileName);
		}
        if (FindNextFileA(handle, &find_data) == 0)
        {
            //if (GetLastError() == ERROR_NO_MORE_FILES)
            break;
        }
	}
}

void ConvertMultibyteToWideChar(std::wstring& out, const std::string& in)
{
    //WideCharToMultiByte
    i32 wide_char_count = MultiByteToWideChar(
        CP_UTF8,                //[in]            UINT                              CodePage,
        MB_ERR_INVALID_CHARS,   //[in]            DWORD                             dwFlags,
        in.c_str(),             //[in]            _In_NLS_string_(cbMultiByte)LPCCH lpMultiByteStr,
        -1,                     //[in]            int                               cbMultiByte,
        nullptr,                //[out, optional] LPWSTR                            lpWideCharStr,
        0                       //[in]            int                               cchWideChar
    );
    assert(wide_char_count > 0);
    out.clear();
    out.resize(wide_char_count);
    i32 wide_char_actual = MultiByteToWideChar(
        CP_UTF8,                //[in]            UINT                              CodePage,
        MB_ERR_INVALID_CHARS,   //[in]            DWORD                             dwFlags,
        in.c_str(),             //[in]            _In_NLS_string_(cbMultiByte)LPCCH lpMultiByteStr,
        -1,                     //[in]            int                               cbMultiByte,
        out.data(),             //[out, optional] LPWSTR                            lpWideCharStr,
        wide_char_count         //[in]            int                               cchWideChar
    );
    assert(wide_char_actual > 0);
    assert(wide_char_actual == wide_char_count);
}

void ConvertWideCharToMultiByte(std::string& out, const std::wstring& in)
{
    //WideCharToMultiByte
    BOOL invalid_string;

    i32 multibyte_char_count = WideCharToMultiByte(
        CP_UTF8,                //[in]            UINT                               CodePage,
        MB_ERR_INVALID_CHARS,   //[in]            DWORD                              dwFlags,
        in.c_str(),             //[in]            _In_NLS_string_(cchWideChar)LPCWCH lpWideCharStr,
        -1,                     //[in]            int                                cchWideChar,
        nullptr,                //[out, optional] LPSTR                              lpMultiByteStr,
        0,                      //[in]            int                                cbMultiByte,
        "#",                    //[in, optional]  LPCCH                              lpDefaultChar,
        LPBOOL(&invalid_string) //[out, optional] LPBOOL                             lpUsedDefaultChar
    );
    assert(multibyte_char_count > 0);
    out.clear();
    out.resize(multibyte_char_count);
    i32 multibyte_char_actual = WideCharToMultiByte(
        CP_UTF8,                //[in]            UINT                               CodePage,
        MB_ERR_INVALID_CHARS,   //[in]            DWORD                              dwFlags,
        in.c_str(),             //[in]            _In_NLS_string_(cchWideChar)LPCWCH lpWideCharStr,
        -1,                     //[in]            int                                cchWideChar,
        out.data(),             //[out, optional] LPSTR                              lpMultiByteStr,
        out.size(),             //[in]            int                                cbMultiByte,
        "#",                    //[in, optional]  LPCCH                              lpDefaultChar,
        LPBOOL(&invalid_string) //[out, optional] LPBOOL                             lpUsedDefaultChar
    );
    assert(multibyte_char_actual > 0);
    assert(multibyte_char_actual == multibyte_char_count);
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

void Sleep_Thread(int64_t milliseconds)
{
    Sleep(1000);
}
