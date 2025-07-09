// HotMon.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// Copyright (c) MMC©2025. All rights reserved.
//
// Hotkey Monitor v0.9 - CLI Search, Logging, App Detection, and more.

#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <map>
#include <string>
#include <algorithm>
#include <sstream>
#include <set>

#pragma comment(lib, "psapi.lib")

#define COLOR_RESET     "\x1b[0m"
#define COLOR_GRAY      "\x1b[90m"
#define COLOR_GREEN     "\x1b[92m"
#define COLOR_YELLOW    "\x1b[93m"
#define COLOR_RED       "\x1b[91m"
#define COLOR_BOLD      "\x1b[97m"

struct HotkeyLog {
    std::string combo;
    std::string timestamp;
    std::string app;
};

std::vector<HotkeyLog> logHistory;
bool ctrl = false, alt = false, shift = false, win = false;
HHOOK keyboardHook;
std::map<std::string, bool> hotkeyAvailability;

std::string GetTimeNow() {
    time_t now = time(nullptr);
    char buf[64];

#pragma warning(suppress: 4996)
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buf);
}

std::string FormatCombo(int vkCode) {
    std::string combo;
    if (ctrl) combo += "Ctrl + ";
    if (alt) combo += "Alt + ";
    if (shift) combo += "Shift + ";
    if (win) combo += "Win + ";

    char name[64];
    UINT sc = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC) << 16;
    if (vkCode >= VK_LEFT && vkCode <= VK_DOWN) sc |= 0x1000000;
    GetKeyNameTextA(sc, name, sizeof(name));
    combo += name;
    return combo;
}

void SaveLogToCSV(const std::string& filename) {
    std::ofstream out(filename);
    out << "Timestamp,Hotkey,Application\n";
    for (const auto& e : logHistory) {
        out << e.timestamp << "," << e.combo << "," << e.app << "\n";
    }
    out.close();
    std::cout << COLOR_YELLOW << "[Saved to " << filename << "]" << COLOR_RESET << "\n";
}

void SaveAvailabilityReport(const std::string& filename) {
    std::ofstream out(filename);
    out << "Hotkey,Available\n";
    for (const auto& kv : hotkeyAvailability) {
        out << kv.first << "," << (kv.second ? "Yes" : "Taken") << "\n";
    }
    out.close();
    std::cout << COLOR_YELLOW << "[Saved availability to " << filename << "]" << COLOR_RESET << "\n";
}

void ProbeHotkeys() {
    std::cout << COLOR_YELLOW << "[Probing Ctrl/Alt/Shift/Win + A-Z, F1-F12, Space...]" << COLOR_RESET << "\n";
    int id = 1000;

    std::vector<UINT> mods = {
        0, MOD_CONTROL, MOD_ALT, MOD_SHIFT, MOD_WIN,
        MOD_CONTROL | MOD_SHIFT,
        MOD_CONTROL | MOD_ALT,
        MOD_ALT | MOD_SHIFT,
        MOD_CONTROL | MOD_ALT | MOD_SHIFT,
        MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_WIN
    };

    std::vector<int> keys;
    for (char c = 'A'; c <= 'Z'; ++c) keys.push_back(c);
    for (int f = VK_F1; f <= VK_F12; ++f) keys.push_back(f);
    keys.push_back(VK_SPACE);

    for (int key : keys) {
        for (UINT mod : mods) {
            std::string combo;
            if (mod & MOD_CONTROL) combo += "Ctrl + ";
            if (mod & MOD_ALT) combo += "Alt + ";
            if (mod & MOD_SHIFT) combo += "Shift + ";
            if (mod & MOD_WIN) combo += "Win + ";

            char name[64];
            UINT sc = MapVirtualKey(key, MAPVK_VK_TO_VSC) << 16;
            if (key >= VK_LEFT && key <= VK_DOWN) sc |= 0x1000000;
            GetKeyNameTextA(sc, name, sizeof(name));
            combo += name;

            if (RegisterHotKey(NULL, id, mod, key)) {
                hotkeyAvailability[combo] = true;
                UnregisterHotKey(NULL, id);
                // std::cout << COLOR_GREEN << "Available: " << combo << COLOR_RESET << "\n";
            }
            else {
                hotkeyAvailability[combo] = false;
                // std::cout << COLOR_RED << "Taken:    " << combo << COLOR_RESET << "\n";
            }
            ++id;
        }
    }

    SaveAvailabilityReport("hotkey_available.csv");
}

// Parsing a combo string into a set of modifiers and the key
void ParseCombo(const std::string& combo, std::set<std::string>& mods, std::string& key) {
    std::istringstream iss(combo);
    std::string token;
    std::vector<std::string> parts;
    while (std::getline(iss, token, '+')) {
        // Remove leading/trailing spaces
        size_t start = token.find_first_not_of(" ");
        size_t end = token.find_last_not_of(" ");
        if (start != std::string::npos)
            token = token.substr(start, end - start + 1);
        else
            token = "";
        parts.push_back(token);
    }
    for (size_t i = 0; i + 1 < parts.size(); ++i) {
        mods.insert(parts[i]);
    }
    if (!parts.empty())
		key = parts.back(); // Last part is the key
}

// Make the parsed combos into Case-insensitive
int strcasecmp(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        if (tolower(*s1) != tolower(*s2))
            return tolower(*s1) - tolower(*s2);
        ++s1; ++s2;
    }
    return *s1 - *s2;
}

// This function will search through the hotkeyAvailability map and print out matching hotkeys
void SearchHotkeys(const std::string& term) {
    // If the map is empty, probe first
    if (hotkeyAvailability.empty()) {
        std::cout << COLOR_YELLOW << "[No hotkey data found. Probing hotkeys first...]" << COLOR_RESET << "\n";
        ProbeHotkeys();
    }
    std::cout << COLOR_YELLOW << "[Searching for: " << term << "]" << COLOR_RESET << "\n";

    // Parse search term
    std::set<std::string> searchMods;
    std::string searchKey;
    ParseCombo(term, searchMods, searchKey);

    bool found = false;
    for (const auto& kv : hotkeyAvailability) {
        std::set<std::string> comboMods;
        std::string comboKey;
        ParseCombo(kv.first, comboMods, comboKey);

        if (comboMods == searchMods &&
            strcasecmp(comboKey.c_str(), searchKey.c_str()) == 0) {
            std::cout << (kv.second ? COLOR_GREEN : COLOR_RED)
                << (kv.second ? "Available: " : "Taken:    ")
                << kv.first << COLOR_RESET << "\n";
            found = true;
        }
    }
    if (!found) {
        std::cout << COLOR_RED << "No exact match found." << COLOR_RESET << "\n";
    }

    /*std::string low = term;
    std::transform(low.begin(), low.end(), low.begin(), ::tolower);
    for (const auto& kv : hotkeyAvailability) {
        std::string key = kv.first;
        std::string keyLow = key;
        std::transform(keyLow.begin(), keyLow.end(), keyLow.begin(), ::tolower);
        if (keyLow.find(low) != std::string::npos) {
            std::cout << (kv.second ? COLOR_GREEN : COLOR_RED)
                << (kv.second ? "Available: " : "Taken:    ")
                << kv.first << COLOR_RESET << "\n";
        }
    }*/
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        int vk = p->vkCode;

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            if (vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL) ctrl = true;
            if (vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU) alt = true;
            if (vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT) shift = true;
            if (vk == VK_LWIN || vk == VK_RWIN) win = true;

            if (ctrl && vk == VK_F10) SaveLogToCSV("hotkey_log.csv");
            if (ctrl && vk == VK_F11) ProbeHotkeys();
            if (vk == VK_ESCAPE) PostQuitMessage(0);

            if (!(vk >= VK_SHIFT && vk <= VK_RWIN)) {
                if (ctrl || alt || shift || win || (vk >= VK_F1 && vk <= VK_F24) || vk == VK_SPACE) {
                    std::string combo = FormatCombo(vk);
                    std::string timestamp = GetTimeNow();

                    HWND hwnd = GetForegroundWindow();
                    DWORD pid;
                    GetWindowThreadProcessId(hwnd, &pid);
                    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
                    char exeName[MAX_PATH] = "Unknown";
                    if (hProc) {
                        GetModuleBaseNameA(hProc, NULL, exeName, MAX_PATH);
                        CloseHandle(hProc);
                    }

                    std::cout << COLOR_GRAY << "[" << timestamp << "] " << COLOR_GREEN
                        << combo << COLOR_RESET << " by " << COLOR_YELLOW << exeName << COLOR_RESET << "\n";

                    logHistory.push_back({ combo, timestamp, exeName });
                }
            }
        }
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            if (vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL) ctrl = false;
            if (vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU) alt = false;
            if (vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT) shift = false;
if (vk == VK_LWIN || vk == VK_RWIN) win = false;
        }
    }
 return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

void RunLiveMonitor() {
    std::cout << COLOR_BOLD << "Hotkey Monitor v0.9" << COLOR_RESET << "\n";
    std::cout << " - Ctrl + F10: Export CSV\n";
    std::cout << " - Ctrl + F11: Probe hotkey availability + autosave\n";
    std::cout << " - ESC: Exit program\n";
    std::cout << COLOR_YELLOW << "Probing will auto-save available/taken hotkeys to 'hotkey_available.csv'" << COLOR_RESET << "\n\n";

    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!keyboardHook) {
        std::cerr << COLOR_RED << "ERROR: Hook install failed." << COLOR_RESET << "\n";
        exit(1);
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(keyboardHook);
}

void EnableAnsiColors() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }
}

int main(int argc, char* argv[]) {
    EnableAnsiColors();

    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--probe" || arg == "-p") {
            ProbeHotkeys();
            return 0;
        }
        /*
		* TODO: active log quitely and ...
        else if (arg == "--save-log" || arg == "-s") {
		    std::cout << COLOR_YELLOW << "To save log u need an active log" << COLOR_RESET << "\n";
            SaveLogToCSV("hotkey_log.csv");
            return 0;
        }*/
        else if ((arg == "--search" || arg == "-f") && argc > 2 &&
            (std::string(argv[2]) == "?" || std::string(argv[2]) == "--help" || std::string(argv[2]) == "-h")) {
            std::cout << "Search hotkey combos by exact modifiers and key.\n"
                << "Format: [Modifier + ...] Key (case-insensitive)\n"
                << "Examples:\n"
                << "  -f \"Win + Space\"\n"
                << "  -f \"Ctrl + Shift + A\"\n"
                << "  -f \"Alt + F4\"\n"
                << "Modifiers: Ctrl, Alt, Shift, Win\n";
            return 0;
        }
        else if ((arg == "--search" || arg == "-f") && argc > 2) {
            SearchHotkeys(argv[2]);
            return 0;
        }
else if (arg == "--live-log" || arg == "-l" || arg == "-i") {
            RunLiveMonitor();
            return 0;
        }
        else if (arg == "--help" || arg == "-h") {
            std::cout << COLOR_BOLD << "HotMon - A Windows Hotkey Monitor CLI v0.9" << COLOR_RESET << "\n";
            std::cout << "HotMon Usage:\n"
                << "  --probe, -p                        Probe available hotkeys and store\n"
                /*<< "  --save-log, -s                   Save current log to CSV\n"*/
                << "  --search, -f <kw> ?/-h/--help      Search hotkey combo\n"
                << "  --live-log, -l, -i                 Start live or Interactive monitor\n"
                << "  --help, -h                         Show this help message\n";
            return 0;
        }
        else {
            std::cout << COLOR_RED << "Unknown argument. Use --help." << COLOR_RESET << "\n";
            return 1;
        }
    }

    RunLiveMonitor();
    return 0;
}

// Visual Studio Desktop C++ Workload installed.
// Visual Studio 2022 or later is recommended.


//   All rights reserved MMC©2025.
//   This code is licensed under the MIT License.