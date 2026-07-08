#include "ManualMap.h"
#include "Utils.h"

// Затирание PE заголовков
bool ErasePEHeaders(HANDLE hProcess, void* remoteBase)
{
    BYTE zero[0x1000] = { 0 };
    SIZE_T written = 0;
    if (WriteProcessMemory(hProcess, remoteBase, zero, 0x1000, &written)) {
        Utils::Success("PE headers erased");
        return true;
    }
    Utils::Error("Failed to erase PE headers");
    return false;
}

// Обычный Manual Map
bool ManualMapInject(HANDLE hProcess, const std::vector<uint8_t>& dllBuffer)
{
    if (dllBuffer.empty()) {
        Utils::Error("DLL buffer is empty");
        return false;
    }

    auto* dos = (IMAGE_DOS_HEADER*)dllBuffer.data();
    auto* nt = (IMAGE_NT_HEADERS64*)(dllBuffer.data() + dos->e_lfanew);

    if (dos->e_magic != IMAGE_DOS_SIGNATURE || nt->Signature != IMAGE_NT_SIGNATURE) {
        Utils::Error("Invalid PE file");
        return false;
    }

    SIZE_T size = nt->OptionalHeader.SizeOfImage;
    void* remoteBase = VirtualAllocEx(hProcess, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    if (!remoteBase) {
        Utils::Error("VirtualAllocEx failed");
        return false;
    }

    WriteProcessMemory(hProcess, remoteBase, dllBuffer.data(), nt->OptionalHeader.SizeOfHeaders, nullptr);

    auto* section = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, section++) {
        if (section->SizeOfRawData) {
            WriteProcessMemory(hProcess,
                (void*)((uintptr_t)remoteBase + section->VirtualAddress),
                dllBuffer.data() + section->PointerToRawData,
                section->SizeOfRawData, nullptr);
        }
    }

    Utils::Success("DLL successfully mapped");

    void* entryPoint = (void*)((uintptr_t)remoteBase + nt->OptionalHeader.AddressOfEntryPoint);

    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)entryPoint, remoteBase, 0, nullptr);
    if (hThread) {
        WaitForSingleObject(hThread, 4000);
        CloseHandle(hThread);
        Utils::Success("Injection completed!");
        return true;
    }

    Utils::Error("CreateRemoteThread failed");
    return false;
}

// СКРЫТНЫЙ Manual Map (Stealth)
bool ManualMapInjectStealth(HANDLE hProcess, const std::vector<uint8_t>& dllBuffer)
{
    if (dllBuffer.empty()) {
        Utils::Error("DLL buffer is empty");
        return false;
    }

    auto* dos = (IMAGE_DOS_HEADER*)dllBuffer.data();
    auto* nt = (IMAGE_NT_HEADERS64*)(dllBuffer.data() + dos->e_lfanew);

    if (dos->e_magic != IMAGE_DOS_SIGNATURE || nt->Signature != IMAGE_NT_SIGNATURE) {
        Utils::Error("Invalid PE file");
        return false;
    }

    SIZE_T size = nt->OptionalHeader.SizeOfImage;

    // Сначала выделяем как RW (не сразу RWX — меньше подозрений)
    void* remoteBase = VirtualAllocEx(hProcess, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteBase) {
        Utils::Error("VirtualAllocEx failed");
        return false;
    }
    Utils::Success("Memory allocated (RW)");

    // Копируем заголовки
    WriteProcessMemory(hProcess, remoteBase, dllBuffer.data(), nt->OptionalHeader.SizeOfHeaders, nullptr);

    // Копируем секции
    auto* section = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, section++) {
        if (section->SizeOfRawData) {
            WriteProcessMemory(hProcess,
                (void*)((uintptr_t)remoteBase + section->VirtualAddress),
                dllBuffer.data() + section->PointerToRawData,
                section->SizeOfRawData, nullptr);
        }
    }
    Utils::Success("Sections mapped");

    // Меняем защиту секций правильно (по флагам)
    section = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, section++) {
        if (section->SizeOfRawData == 0) continue;

        DWORD oldProtect = 0;
        DWORD newProtect = PAGE_READONLY;

        bool exec = section->Characteristics & IMAGE_SCN_MEM_EXECUTE;
        bool read = section->Characteristics & IMAGE_SCN_MEM_READ;
        bool write = section->Characteristics & IMAGE_SCN_MEM_WRITE;

        if (exec && read && write)      newProtect = PAGE_EXECUTE_READWRITE;
        else if (exec && read)          newProtect = PAGE_EXECUTE_READ;
        else if (exec)                  newProtect = PAGE_EXECUTE;
        else if (read && write)         newProtect = PAGE_READWRITE;
        else if (read)                  newProtect = PAGE_READONLY;

        VirtualProtectEx(hProcess,
            (void*)((uintptr_t)remoteBase + section->VirtualAddress),
            section->Misc.VirtualSize,
            newProtect, &oldProtect);
    }
    Utils::Success("Section protection restored");

    // Затираем PE заголовки для скрытности
    ErasePEHeaders(hProcess, remoteBase);

    // Вызов DllMain
    void* entryPoint = (void*)((uintptr_t)remoteBase + nt->OptionalHeader.AddressOfEntryPoint);

    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
        (LPTHREAD_START_ROUTINE)entryPoint, remoteBase, 0, nullptr);

    if (hThread) {
        WaitForSingleObject(hThread, 4000);
        CloseHandle(hThread);
        Utils::Success("Stealth injection completed!");
        return true;
    }

    Utils::Error("CreateRemoteThread failed");
    return false;
}