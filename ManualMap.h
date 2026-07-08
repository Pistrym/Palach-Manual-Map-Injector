#pragma once
#include "pch.h"

bool ManualMapInject(HANDLE hProcess, const std::vector<uint8_t>& dllBuffer);
bool ManualMapInjectStealth(HANDLE hProcess, const std::vector<uint8_t>& dllBuffer);
bool ErasePEHeaders(HANDLE hProcess, void* remoteBase);