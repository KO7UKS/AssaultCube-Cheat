#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <csignal>
#include <cstdlib>

// Функция для получения ID процесса по его имени
DWORD GetProcessIdByName(const wchar_t* processName) {
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return 0;
    }

    if (Process32First(hProcessSnap, &processEntry)) {
        do {
            if (!wcscmp(processEntry.szExeFile, processName)) {
                CloseHandle(hProcessSnap);
                return processEntry.th32ProcessID;
            }
        } while (Process32Next(hProcessSnap, &processEntry));
    }

    CloseHandle(hProcessSnap);
    return 0;
}

// Функция для применения патча памяти
bool PatchMemory(HANDLE hProcess, LPCVOID address, const BYTE* patchBytes, SIZE_T size) {
    DWORD oldProtect;
    if (VirtualProtectEx(hProcess, (LPVOID)address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        WriteProcessMemory(hProcess, (LPVOID)address, patchBytes, size, NULL);
        VirtualProtectEx(hProcess, (LPVOID)address, size, oldProtect, &oldProtect);
        return true;
    }
    return false;
}

// Глобальные переменные
HANDLE hProcess = NULL;

// Адреса и патчи для F1 и F2
const LPCVOID addressToPatchF1 = (LPCVOID)0x004C73EF; // Адрес для F1
const BYTE nopPatchF1[6] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }; // Патч для F1 (NOP)
BYTE originalBytesF1[6]; // Оригинальные байты для F1
bool isPatchedF1 = false; // Статус патча для F1

const LPCVOID addressToPatchF2 = (LPCVOID)0x0041C223; // Адрес для F2
const BYTE nopPatchF2[3] = { 0x90, 0x90, 0x90 }; // Патч для F2 (NOP)
BYTE originalBytesF2[3]; // Оригинальные байты для F2
bool isPatchedF2 = false; // Статус патча для F2

// Восстановление оригинальных байт
void RestoreOriginalBytes(LPCVOID addressToPatch, BYTE* originalBytes, SIZE_T size, bool& isPatched) {
    if (isPatched && hProcess != NULL) {
        PatchMemory(hProcess, addressToPatch, originalBytes, size);
        std::wcout << L"OFF.\n";
        isPatched = false;
    }
}

// Очистка ресурсов и восстановление байт
void Cleanup() {
    RestoreOriginalBytes(addressToPatchF1, originalBytesF1, sizeof(originalBytesF1), isPatchedF1);
    RestoreOriginalBytes(addressToPatchF2, originalBytesF2, sizeof(originalBytesF2), isPatchedF2);
    if (hProcess != NULL) {
        CloseHandle(hProcess);
    }
}

// Обработчик закрытия консоли
BOOL WINAPI ConsoleHandler(DWORD signal) {
    Cleanup();
    exit(0);
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "ru_RU.UTF-8");

    // Установка обработчика сигнала закрытия консоли
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    // Получение ID процесса
    DWORD processId = GetProcessIdByName(L"ac_client.exe");
    if (processId == 0) {
        std::wcout << L"Не удалось найти процесс игры.\n";
        return 1;
    }

    // Открытие процесса для доступа
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL) {
        std::wcout << L"Не удалось открыть процесс игры.\n";
        return 1;
    }

    // Чтение оригинальных байт по адресам для F1 и F2
    if (!ReadProcessMemory(hProcess, addressToPatchF1, originalBytesF1, sizeof(originalBytesF1), NULL)) {
        std::wcout << L"Ошибка чтения оригинальных байт (F1).\n";
        CloseHandle(hProcess);
        return 1;
    }

    if (!ReadProcessMemory(hProcess, addressToPatchF2, originalBytesF2, sizeof(originalBytesF2), NULL)) {
        std::wcout << L"Ошибка чтения оригинальных байт (F2).\n";
        CloseHandle(hProcess);
        return 1;
    }

    std::wcout << L"Нажмите F1 для включения/выключения Бесконечных Патронов.\n";
    std::wcout << L"Нажмите F2 для включения/выключения Бесконечных ХП.\n";

    // Основной цикл программы
    while (true) {
        // Обработка нажатия F1
        if (GetAsyncKeyState(VK_F1) & 1) {
            if (!isPatchedF1) {
                if (PatchMemory(hProcess, addressToPatchF1, nopPatchF1, sizeof(nopPatchF1))) {
                    std::wcout << L"Бесконечные Патроны активированы.\n";
                    isPatchedF1 = true;
                }
            }
            else {
                RestoreOriginalBytes(addressToPatchF1, originalBytesF1, sizeof(originalBytesF1), isPatchedF1);
            }
        }

        // Обработка нажатия F2
        if (GetAsyncKeyState(VK_F2) & 1) {
            if (!isPatchedF2) {
                if (PatchMemory(hProcess, addressToPatchF2, nopPatchF2, sizeof(nopPatchF2))) {
                    std::wcout << L"Бесконечные ХП активированы.\n";
                    isPatchedF2 = true;
                }
            }
            else {
                RestoreOriginalBytes(addressToPatchF2, originalBytesF2, sizeof(originalBytesF2), isPatchedF2);
            }
        }

    }

    return 0; // Завершение программы
}
