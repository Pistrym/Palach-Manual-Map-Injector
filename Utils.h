#pragma once
#include "pch.h"

class Utils
{
public:
    static DWORD GetProcessId(const std::wstring& processName);
    static std::vector<uint8_t> ReadFile(const std::string& path);
    static void EnableDebugPriv();
    static void Success(const char* text);
    static void Error(const char* text);
};