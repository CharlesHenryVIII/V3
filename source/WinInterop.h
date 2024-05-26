#pragma once
#include <string>
#include <thread>
#include <vector>

bool CreateFolder(const std::string& folderLocation);
void DebugPrint(const char* fmt, ...);
void DebugPrint(const wchar_t* fmt, ...);
std::string ToString(const char* fmt, ...);
void ScanDirectoryForFileNames(const std::string& dir, std::vector<std::string>& out);
void SetThreadName(std::thread::native_handle_type threadID, std::string name);
void Sleep_Thread(int64_t milliseconds);
void ConvertMultibyteToWideChar(std::wstring& out, const std::string& in);
void ConvertWideCharToMultiByte(std::string& out, const std::wstring& in);
