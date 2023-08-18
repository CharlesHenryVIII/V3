#include "WinInterop_File.h"
#include "WinInterop.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

File::File(char const* filename, File::Mode fileMode, bool createIfNotFound)
{
    std::string sFileName = std::string(filename);
    Init(sFileName, fileMode, createIfNotFound);
}

File::File(const std::string& filename, File::Mode fileMode, bool createIfNotFound)
{
    Init(filename, fileMode, createIfNotFound);
}

void File::GetHandle()
{
    //MultiByteToWideChar(CP_UTF8, );
    //TCHAR* test = TEXT("TEST");
    m_handle = CreateFile(m_filename.c_str(), m_accessType, m_shareType,
        NULL, m_openType, FILE_ATTRIBUTE_NORMAL, NULL);
}

void File::Init(const std::string& filename, File::Mode fileMode, bool createIfNotFound)
{
    m_filename = std::string(filename);
    m_accessType = GENERIC_READ;
    m_shareType  = FILE_SHARE_READ;
    m_openType   = OPEN_EXISTING;
    //m_fileAttribute = FILE_ATTRIBUTE_NORMAL;

    switch (fileMode)
    {
    case File::Mode::Read:
        m_accessType = GENERIC_READ;
        m_shareType  = FILE_SHARE_READ;
        //m_fileAttribute = FILE_ATTRIBUTE_READONLY;
        break;
    case File::Mode::Write:
        m_openType = TRUNCATE_EXISTING;
        //[[fallthrough]];
    //case File::Mode::Append:
        m_accessType = GENERIC_WRITE;
        m_shareType = FILE_SHARE_WRITE;
        break;
    default:
        break;
    }
    GetHandle();

    if (createIfNotFound && m_handle == INVALID_HANDLE_VALUE)
    {
        m_openType = CREATE_NEW;
        GetHandle();
    }
    m_handleIsValid = (m_handle != INVALID_HANDLE_VALUE);
    //assert(m_handleIsValid);
    auto filePointerLocation = FILE_END;

    if (m_handleIsValid)
    {
        switch (fileMode)
        {
        case File::Mode::Read:
            filePointerLocation = FILE_BEGIN;
            [[fallthrough]];
        //case File::Mode::Append:
        //    break;
        default:
            break;
        }
    }

    DWORD newFilePointer = SetFilePointer(m_handle, 0, NULL, filePointerLocation);
}

bool File::FileDestructor()
{
    return CloseHandle(m_handle);
}

File::~File()
{
    if (m_handleIsValid)
    {
        FileDestructor();
    }
}


void File::GetText()
{
    if (!m_handleIsValid)
        return;

    u32 bytesRead;
    static_assert(sizeof(DWORD) == sizeof(u32));
    static_assert(sizeof(LPVOID) == sizeof(void*));

    const u32 fileSize = GetFileSize(m_handle, NULL);
    m_dataString.resize(fileSize, 0);
    m_textIsValid = true;
    if (!ReadFile(m_handle, (LPVOID)m_dataString.c_str(), (DWORD)fileSize, reinterpret_cast<LPDWORD>(&bytesRead), NULL))
    {
        //assert(false);
        m_textIsValid = false;
    }
}

void File::GetData()
{
    if (!m_handleIsValid)
        return;

    u32 bytesRead;
    static_assert(sizeof(DWORD) == sizeof(u32));
    static_assert(sizeof(LPVOID) == sizeof(void*));

    const u32 fileSize = GetFileSize(m_handle, NULL);
    m_dataBinary.resize(fileSize, 0);
    m_binaryDataIsValid = true;
    if (ReadFile(m_handle, (LPVOID)m_dataBinary.data(), (DWORD)fileSize, reinterpret_cast<LPDWORD>(&bytesRead), NULL) == 0)
    {
        m_binaryDataIsValid = false;
        DWORD error = GetLastError();
    }
}
bool File::Write(void* data, size_t sizeInBytes)
{
    DWORD bytesWritten = {};
    BOOL result = WriteFile(m_handle, data, (DWORD)sizeInBytes, &bytesWritten, NULL);
    return result != 0;
}

bool File::Write(const void* data, size_t sizeInBytes)
{
    DWORD bytesWritten = {};
    BOOL result = WriteFile(m_handle, data, (DWORD)sizeInBytes, &bytesWritten, NULL);
    return result != 0;
}

bool File::Write(const std::string& text)
{
    DWORD bytesWritten = {};
    BOOL result = WriteFile(m_handle, text.c_str(), (DWORD)text.size(), &bytesWritten, NULL);
    return result != 0;
    //int32 result = fputs(text.c_str(), m_handle);
    //return (!(result == EOF));
    //return false;
}

void File::GetTime()
{
    FILETIME creationTime;
    FILETIME lastAccessTime;
    FILETIME lastWriteTime;
    if (!GetFileTime(m_handle, &creationTime, &lastAccessTime, &lastWriteTime))
    {
        printf("GetFileTime failed with %d\n", GetLastError());
        m_timeIsValid = false;
    }
    else
    {
        ULARGE_INTEGER actualResult;
        actualResult.LowPart = lastWriteTime.dwLowDateTime;
        actualResult.HighPart = lastWriteTime.dwHighDateTime;
        m_time = actualResult.QuadPart;
        m_timeIsValid = true;
    }
}

bool File::Delete()
{
    if (m_handleIsValid)
    {
        FileDestructor();
        //std::wstring fuckingWide(m_filename.begin(), m_filename.end());
        bool result = DeleteFile(m_filename.c_str());
        m_handleIsValid = false;
        return result;
    }
    return false;
}
