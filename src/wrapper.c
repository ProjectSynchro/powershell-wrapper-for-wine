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

// Function to replace occurrences of a substring in a string
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

__attribute__((externally_visible)) // for -fwhole-program
int mainCRTStartup(void) {
    BOOL read_from_stdin = FALSE;
    wchar_t cmdlineW[4096] = L"", pwsh_pathW[MAX_PATH] = L"", bufW[MAX_PATH] = L"", **argv;
    DWORD exitcode;
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi = {0};
    int i, j, argc;

    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    ExpandEnvironmentStringsW(L"%SystemDrive%\\Program Files\\PowerShell\\7\\pwsh.exe", pwsh_pathW, MAX_PATH + 1);

    // Set environment variable to disable color rendering output 
    _wputenv_s(L"NO_COLOR", L"1");
    // or _wputenv_s(L"TERM", L"xterm-mono");

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
        wcscat(wcscat(cmdlineW, L" "), argv[j]);
    }

    // Insert '-c' if necessary
    if (argv[i] && _wcsnicmp(argv[i - 1], L"-c", 2) && _wcsnicmp(argv[i - 1], L"-enc", 4) &&
        _wcsnicmp(argv[i - 1], L"-f", 2) && _wcsnicmp(argv[i], L"/c", 2)) {
        wcscat(cmdlineW, L" -c ");
    }

    // Concatenate the rest of the arguments into the new cmdline
    for (j = i; j < argc; j++) {
        wcscat(wcscat(cmdlineW, L" "), argv[j]);
    }

    // Handle pipeline for reading from stdin
    if (read_from_stdin) {
        // Handle pipe
        if (GetFileType(GetStdHandle(STD_INPUT_HANDLE)) != FILE_TYPE_CHAR) {
            if (!wcscmp(argv[argc - 1], L"-") && _wcsnicmp(argv[argc - 2], L"-c", 2)) {
                wcscat(cmdlineW, L" -c ");
            }
        }
    }

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

    // Execute the command through pwsh.exe
    CreateProcessW(pwsh_pathW, cmdlineW, 0, 0, 0, 0, 0, 0, &si, &pi);
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exitcode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    LocalFree(argv);

    exit(exitcode);
}
