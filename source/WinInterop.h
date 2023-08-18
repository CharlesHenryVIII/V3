#pragma once
#include "Math.h"
#include "WinInterop_File.h"

#include <string>
#include <thread>

bool CreateFolder(const std::string& folderLocation);
void DebugPrint(const char* fmt, ...);
std::string ToString(const char* fmt, ...);
void SetThreadName(std::thread::native_handle_type threadID, std::string name);
void Sleep_Thread(i64 milliseconds);
