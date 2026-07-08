#include "pch.h"
#include "Utils.h"
#include "ManualMap.h"

#define WHITE  "\x1B[97m"
#define GRAY   "\x1B[90m"
#define BOLD   "\x1B[1m"

void EnableVirtualTerminal()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void PrintBanner()
{
    system("cls");
    std::cout << WHITE << BOLD;
    std::cout << "  ____   _    _        _    ____ _   _ \n";
    std::cout << " |  _ \\ / \\  | |      / \\  / ___| | | |\n";
    std::cout << " | |_) / _ \\ | |     / _ \\| |   | |_| |\n";
    std::cout << " |  __/ ___ \\| |___ / ___ \\ |___|  _  |\n";
    std::cout << " |_| /_/   \\_\\_____/_/   \\_\\____|_| |_|\n";
    std::cout << RESET;
    std::cout << WHITE << "        >> Manual Map Injector v1.0 <<\n" << RESET;
    std::cout << GRAY << "        ==============================\n\n" << RESET;
}

void PrintLine()
{
    std::cout << GRAY << " -------------------------------------------------\n" << RESET;
}

void PrintStatus(const char* label, const char* value)
{
    std::cout << WHITE << " [ " << label << " ] " << value << RESET << "\n";
}

void PrintMenu()
{
    PrintLine();
    std::cout << WHITE << "  [1] Inject DLL           (Normal)\n";
    std::cout << WHITE << "  [2] Stealth Inject       (Hidden)\n";
    std::cout << WHITE << "  [3] Erase PE Headers     (Cleanup)\n";
    std::cout << WHITE << "  [4] About\n";
    std::cout << WHITE << "  [0] Exit\n";
    PrintLine();
    std::cout << WHITE << " Select option: ";
}

void PrintAbout()
{
    system("cls");
    PrintBanner();
    PrintLine();
    std::cout << WHITE << "  Project:  Palach Injector\n";
    std::cout << WHITE << "  Version:  1.0\n";
    std::cout << WHITE << "  Type:     Manual Map Injector\n";
    std::cout << WHITE << "  Author:   Pistrum\n";
    std::cout << WHITE << "  Platform: Windows x64\n";
    PrintLine();
    std::cout << "\n " << WHITE << "Press ENTER to return..." << RESET;
    std::cin.get();
}

void DoInjection(bool stealth)
{
    std::string dllPath;
    std::wstring targetProcess;

    PrintLine();
    std::cout << WHITE << " [?] Path to DLL       > ";
    std::getline(std::cin, dllPath);

    std::cout << WHITE << " [?] Target process    > ";
    std::getline(std::wcin, targetProcess);

    PrintLine();
    if (stealth)
        std::cout << WHITE << " [*] Starting STEALTH injection...\n" << RESET;
    else
        std::cout << WHITE << " [*] Starting NORMAL injection...\n" << RESET;
    PrintLine();

    auto dllData = Utils::ReadFile(dllPath);
    if (dllData.empty()) {
        std::cout << WHITE << " [-] Failed to read DLL file\n" << RESET;
        PrintLine();
        std::cout << WHITE << " Press ENTER to continue..." << RESET;
        std::cin.get();
        return;
    }
    PrintStatus("FILE ", "DLL loaded into buffer");

    DWORD pid = Utils::GetProcessId(targetProcess);
    if (!pid) {
        std::cout << WHITE << " [-] Process not found\n" << RESET;
        PrintLine();
        std::cout << WHITE << " Press ENTER to continue..." << RESET;
        std::cin.get();
        return;
    }

    char pidStr[64];
    sprintf_s(pidStr, "Target PID: %lu", pid);
    PrintStatus("PID  ", pidStr);

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) {
        std::cout << WHITE << " [-] Cannot open target process\n" << RESET;
        PrintLine();
        std::cout << WHITE << " Press ENTER to continue..." << RESET;
        std::cin.get();
        return;
    }
    PrintStatus("PROC ", "Handle acquired");

    PrintLine();
    std::cout << WHITE << " [*] Mapping DLL into remote process...\n" << RESET;

    bool success = stealth
        ? ManualMapInjectStealth(hProc, dllData)
        : ManualMapInject(hProc, dllData);

    if (success) {
        PrintLine();
        std::cout << WHITE << BOLD;
        std::cout << "  ============================================\n";
        std::cout << "         INJECTION SUCCESSFUL!\n";
        std::cout << "  ============================================\n" << RESET;
    }
    else {
        PrintLine();
        std::cout << WHITE << BOLD;
        std::cout << "  ============================================\n";
        std::cout << "         INJECTION FAILED!\n";
        std::cout << "  ============================================\n" << RESET;
    }

    CloseHandle(hProc);
    PrintLine();
    std::cout << WHITE << " Press ENTER to return to menu..." << RESET;
    std::cin.get();
}

void DoErasePE()
{
    std::wstring targetProcess;
    std::string addrStr;

    PrintLine();
    std::cout << WHITE << " [?] Target process    > ";
    std::getline(std::wcin, targetProcess);

    std::cout << WHITE << " [?] Module base (hex) > 0x";
    std::getline(std::cin, addrStr);

    uintptr_t base = 0;
    try { base = std::stoull(addrStr, nullptr, 16); }
    catch (...) {
        std::cout << WHITE << " [-] Invalid address\n" << RESET;
        Sleep(1500);
        return;
    }

    DWORD pid = Utils::GetProcessId(targetProcess);
    if (!pid) {
        std::cout << WHITE << " [-] Process not found\n" << RESET;
        Sleep(1500);
        return;
    }

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) {
        std::cout << WHITE << " [-] Cannot open process\n" << RESET;
        Sleep(1500);
        return;
    }

    PrintLine();
    if (ErasePEHeaders(hProc, (void*)base))
        std::cout << WHITE << " [+] PE Headers cleared\n" << RESET;
    else
        std::cout << WHITE << " [-] Failed\n" << RESET;

    CloseHandle(hProc);
    PrintLine();
    std::cout << WHITE << " Press ENTER to continue..." << RESET;
    std::cin.get();
}

int main()
{
    SetConsoleTitleA("Palach Injector v1.0");
    EnableVirtualTerminal();
    Utils::EnableDebugPriv();

    while (true)
    {
        PrintBanner();

        PrintStatus("PRIV ", "SeDebugPrivilege enabled");
        PrintStatus("ARCH ", "x64");
        PrintStatus("MODE ", "Manual Mapping");

        PrintMenu();

        std::string input;
        std::getline(std::cin, input);

        if (input == "1")      DoInjection(false);
        else if (input == "2") DoInjection(true);
        else if (input == "3") DoErasePE();
        else if (input == "4") PrintAbout();
        else if (input == "0") {
            std::cout << WHITE << "\n [*] Exiting...\n" << RESET;
            break;
        }
        else {
            std::cout << WHITE << " [-] Invalid option\n" << RESET;
            Sleep(1000);
        }
    }

    return 0;
}