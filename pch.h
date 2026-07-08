#pragma once
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <TlHelp32.h>
#include <Psapi.h>

#pragma comment(lib, "ntdll.lib")

#define GREEN   "\x1B[32m"
#define RED     "\x1B[31m"
#define YELLOW  "\x1B[33m"
#define RESET   "\x1B[0m"