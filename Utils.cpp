#include "Utils.h"

DWORD Utils::GetProcessId(const std::wstring& processName)
{
    PROCESSENTRY32W pe32{ sizeof(PROCESSENTRY32W) };
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32FirstW(hSnap, &pe32))
    {
        do
        {
            if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0)
            {
                CloseHandle(hSnap);
                return pe32.th32ProcessID;
            }
        } while (Process32NextW(hSnap, &pe32));
    }
    CloseHandle(hSnap);
    return 0;
}

std::vector<uint8_t> Utils::ReadFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};

    auto size = f.tellg();
    std::vector<uint8_t> buffer(size);
    f.seekg(0);
    f.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

void Utils::EnableDebugPriv()
{
    HANDLE hToken;
    LUID luid;
    TOKEN_PRIVILEGES tp;

    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
    LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid);

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    CloseHandle(hToken);
}

void Utils::Success(const char* text) { std::cout << GREEN << "[+] " << text << RESET << "\n"; }
void Utils::Error(const char* text) { std::cout << RED << "[-] " << text << RESET << "\n"; }