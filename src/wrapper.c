/* Wraps PowerShell command-line into correct syntax for pwsh.exe. */

#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include "shlwapi.h"

// Function to check if an option is a single or last option
static inline BOOL is_single_or_last_option(WCHAR *opt) {
    return ((!_wcsnicmp(opt, L"-c", 2) && _wcsnicmp(opt, L"-config", 7)) || !_wcsnicmp(opt, L"-n", 2) ||
            !_wcsnicmp(opt, L"-f", 2) || !wcscmp(opt, L"-") || !_wcsnicmp(opt, L"-enc", 4) ||
            !_wcsnicmp(opt, L"-m", 2) || !_wcsnicmp(opt, L"-s", 2));
}

#ifdef ENABLE_DEBUG_LOG
// Function to log the Command Line to a File
static void LogCommandLine(LPCWSTR cmdline) {
    HANDLE hFile = CreateFileW(L"C:\\debug.log", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        SetFilePointer(hFile, 0, NULL, FILE_END);
        WCHAR buffer[1024];
        wsprintfW(buffer, L"[%hs %hs] Command line: %ls\r\n", __DATE__, __TIME__, cmdline);
        DWORD written;
        WriteFile(hFile, buffer, lstrlenW(buffer) * sizeof(WCHAR), &written, NULL);
        CloseHandle(hFile);
    }
}

// Define a macro for logging
#define LOG_CMDLINE(cmdline) LogCommandLine(cmdline)
#else
// Define a no-op macro when debugging is disabled
#define LOG_CMDLINE(cmdline) ((void)0)
#endif

/*
Following function taken from https://creativeandcritical.net/downloads/replacebench.c which is in public domain; 
Credits to the there mentioned authors. Replaces in the string "str" all the occurrences of the string "sub" with the string "rep" 
*/
static inline wchar_t *replace_smart(wchar_t *str, wchar_t *sub, wchar_t *rep) {
    size_t slen = wcslen(sub);
    size_t rlen = wcslen(rep);
    size_t size = wcslen(str) + 1;
    size_t diff = rlen - slen;
    size_t capacity = (diff > 0 && slen) ? 2 * size : size;
    wchar_t *buf = (wchar_t *)HeapAlloc(GetProcessHeap(), 8, sizeof(wchar_t) * capacity);
    wchar_t *find, *b = buf;

    if (b == NULL) return NULL;
    if (slen == 0) return memcpy(b, str, sizeof(wchar_t) * size);

    while ((find = StrStrIW(str, sub))) {
        if ((size += diff) > capacity) {
            wchar_t *ptr = (wchar_t *)HeapReAlloc(GetProcessHeap(), 0, buf, capacity = 2 * size * sizeof(wchar_t));
            if (ptr == NULL) {
                HeapFree(GetProcessHeap(), 0, buf);
                return NULL;
            }
            b = ptr + (b - buf);
            buf = ptr;
        }
        memcpy(b, str, (find - str) * sizeof(wchar_t));
        b += find - str;
        memcpy(b, rep, rlen * sizeof(wchar_t));
        b += rlen;
        str = find + slen;
    }
    memcpy(b, str, (size - (b - buf)) * sizeof(wchar_t));
    b = (wchar_t *)HeapReAlloc(GetProcessHeap(), 0, buf, size * sizeof(wchar_t));
    return b ? b : buf;
}

// Function to replace double quotes with single quotes in the cmdline
void fix_quotes(wchar_t *str) {
    // Replace all double quotes with single quotes
    wchar_t *tmp = replace_smart(str, L"\"", L"\'");
    if (tmp) {
        wcscpy(str, tmp);
        HeapFree(GetProcessHeap(), 0, tmp);
    }
}

// Function to check if the process is running under WOW64 (32-bit process on 64-bit Windows)
BOOL is_wow64() {
    BOOL bIsWow64 = FALSE;
    IsWow64Process(GetCurrentProcess(), &bIsWow64);
    return bIsWow64;
}

// Function to get the correct Program Files path
void get_program_files_path(wchar_t *path, size_t size) {
    if (is_wow64()) {
        ExpandEnvironmentStringsW(L"%ProgramW6432%\\PowerShell\\7\\pwsh.exe", path, (DWORD)size);
    } else {
        ExpandEnvironmentStringsW(L"%ProgramFiles%\\PowerShell\\7\\pwsh.exe", path, (DWORD)size);
    }
}

__attribute__((externally_visible)) // for -fwhole-program
int mainCRTStartup(void) {
    BOOL read_from_stdin = FALSE;
    wchar_t cmdlineW[4096] = L"", pwsh_pathW[MAX_PATH] = L"", bufW[MAX_PATH] = L"", **argv;
    DWORD exitcode;
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi = {0};
    int i, j, argc;

    argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    // Get the correct Program Files path based on the process architecture
    get_program_files_path(pwsh_pathW, MAX_PATH + 1);

    // Set environment variable to disable color rendering output 
    _wputenv_s(L"NO_COLOR", L"1");
    // or _wputenv_s(L"TERM", L"xterm-mono");

    // Set environment variable to enable debugging on the powershell side
    #ifdef ENABLE_DEBUG_LOG
    _wputenv_s(L"LOG_DEBUG", L"1");
    #endif

    // Concatenate options into new cmdline, handling some incompatibilities
    for (i = 1; argv[i] && !wcsncmp(argv[i], L"-", 1); i++) {
        if (!is_single_or_last_option(argv[i])) i++;
        if (!argv[i]) break;
    }

    for (j = 1; j < i; j++) {
        if (!wcscmp(L"-", argv[j])) {
            if (j == (argc - 1)) {
                read_from_stdin = TRUE;
                continue;
            } else {
                fputs("Invalid usage", stderr);
                exit(1);
            }
        }
        if (!_wcsnicmp(argv[j], L"-ve", 3)) {
            j++;
            continue;
        }
        if (!_wcsnicmp(argv[j], L"-nop", 4)) {
            continue;
        }
        if (wcschr(argv[j], L' ') && argv[j][0] != L'\"') {
            wcscat(cmdlineW, L" \"");
            wcscat(cmdlineW, argv[j]);
            wcscat(cmdlineW, L"\"");
        } else {
            wcscat(wcscat(cmdlineW, L" "), argv[j]);
        }
    }

    // Insert '-c' if necessary
    if (argv[i] && _wcsnicmp(argv[i - 1], L"-c", 2) && _wcsnicmp(argv[i - 1], L"-enc", 4) &&
        _wcsnicmp(argv[i - 1], L"-f", 2) && _wcsnicmp(argv[i], L"/c", 2)) {
        wcscat(cmdlineW, L" -c ");
    }

    // Concatenate the rest of the arguments into the new cmdline
    for (j = i; j < argc; j++) {
        if (wcschr(argv[j], L' ') && argv[j][0] != L'\"') {
            wcscat(cmdlineW, L" \"");
            wcscat(cmdlineW, argv[j]);
            wcscat(cmdlineW, L"\"");
        } else {
            wcscat(wcscat(cmdlineW, L" "), argv[j]);
        }
    }

    // Support pipeline to handle something like "$(get-date) | powershell -"
    if (read_from_stdin) {
        WCHAR defline[4096]; // Buffer to store converted line
        char line[4096];     // Buffer to store input line
        HANDLE input = GetStdHandle(STD_INPUT_HANDLE); // Get the standard input handle
        DWORD type = GetFileType(input);               // Get the file type of the input handle

        // Check if input is redirected (e.g., via pipe)
        if (type != FILE_TYPE_CHAR) { // Not redirected (FILE_TYPE_PIPE or FILE_TYPE_DISK)
            // Check if the last argument is "-" and the second-to-last argument is not "-c"
            if (!wcscmp(argv[argc - 1], L"-") && _wcsnicmp(argv[argc - 2], L"-c", 2)) {
                wcscat(cmdlineW, L" -c "); // Append "-c" to cmdlineW
            }
            wcscat(cmdlineW, L" "); // Append a space to cmdlineW
            // Read input line by line and append to cmdlineW after converting to wide characters
            while (fgets(line, 4096, stdin) != NULL) {
                mbstowcs(defline, line, 4096); // Convert input line to wide characters
                wcscat(cmdlineW, defline);    // Append converted line to cmdlineW
            }
        }
    } // End support pipeline

    // Replace incompatible commands/strings in the cmdline fed to pwsh.exe
    if (GetEnvironmentVariableW(L"PSHACKS", bufW, MAX_PATH + 1)) {
        WCHAR buf_fromW[MAX_PATH];
        WCHAR buf_toW[MAX_PATH];
        WCHAR *buf_replacedW = NULL;

        if (GetEnvironmentVariableW(L"PS_FROM", buf_fromW, MAX_PATH + 1) &&
            GetEnvironmentVariableW(L"PS_TO", buf_toW, MAX_PATH + 1)) {
            wchar_t *bufferA, *bufferB = 0;

            wchar_t *tokenA = wcstok_s(buf_fromW, L"¶", &bufferA);
            wchar_t *tokenB = wcstok_s(buf_toW, L"¶", &bufferB);

            while (tokenA && tokenB) {
                buf_replacedW = replace_smart(cmdlineW, tokenA, tokenB);
                wcscpy(cmdlineW, buf_replacedW);
                HeapFree(GetProcessHeap(), 0, buf_replacedW);

                tokenA = wcstok_s(NULL, L"¶", &bufferA);
                tokenB = wcstok_s(NULL, L"¶", &bufferB);
            }
        }
    }

    /*
    Replace double quotes with single quotes in the cmdline

    This is for invokations that use double quotes with arguments that have spaces in them
    This causes issues with pwsh parsing the invokation when you call the wrapper directly

    e.g. 
    powershell.exe Start-Process -Verb RunAs -FilePath "path to EasyAntiCheat_EOS_Setup.exe" -ArgumentList "install ****" 
    turns into: 
    pwsh -c Start-Process -Verb RunAs -FilePath 'path to EasyAntiCheat_EOS_Setup.exe' -ArgumentList 'install ****'
    */

    fix_quotes(cmdlineW);

    // **Debugging: Log the Command Line**
    LOG_CMDLINE(cmdlineW);

    // Execute the command through pwsh.exe
    CreateProcessW(pwsh_pathW, cmdlineW, 0, 0, 0, 0, 0, 0, &si, &pi);
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exitcode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    LocalFree(argv);

    exit(exitcode);
}
