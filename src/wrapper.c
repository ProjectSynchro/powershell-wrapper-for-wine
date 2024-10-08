/* Wraps PowerShell command-line into correct syntax for pwsh.exe. */

#include <wchar.h>
#include <windows.h>
#include <stdio.h>
#include <shlwapi.h>

// Function to get the arguments part of the command line, skipping the executable name
LPCWSTR SkipProgramName(LPCWSTR cmdline) {
    LPCWSTR p = cmdline;

    // Skip leading whitespace
    while (*p && iswspace(*p)) p++;

    if (*p == L'"') {
        p++;
        while (*p) {
            if (*p == L'"') {
                if (*(p + 1) == L'"') {
                    p += 2;
                } else {
                    p++;
                    break;
                }
            } else {
                p++;
            }
        }
    } else {
        while (*p && !iswspace(*p)) p++;
    }

    // Skip whitespace between program name and arguments
    while (*p && iswspace(*p)) p++;

    return p;
}

/*
Following function taken from https://creativeandcritical.net/downloads/replacebench.c which is in public domain;
Credits to the there mentioned authors. Replaces in the string "str" all the occurrences of the string "sub" with the string "rep".
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

// Function to determine if '-c' should be added to the command line
BOOL should_add_c_option(LPCWSTR args) {
    // Check if the first argument starts with '-' or '/'
    if (*args == L'-' || *args == L'/') {
        LPCWSTR opt = args + 1;
        if (!_wcsnicmp(opt, L"c", 1) || !_wcsnicmp(opt, L"Command", 7) ||
            !_wcsnicmp(opt, L"enc", 3) || !_wcsnicmp(opt, L"EncodedCommand", 14) ||
            !_wcsnicmp(opt, L"file", 4) || !_wcsnicmp(opt, L"f", 1)) {
            return FALSE;
        }
    }
    return TRUE;
}

// Function to check if input is coming from a pipeline
BOOL is_input_from_pipeline() {
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    if (!GetConsoleMode(hStdin, &mode)) {
        // If GetConsoleMode fails, it means the input is not from a console, so it's from a pipeline
        return TRUE;
    }
    return FALSE;
}

// Function to escape double quotes and remove unnecessary single quotes
void escape_and_fix_quotes(LPCWSTR input, LPWSTR output, size_t output_size) {
    // Step 1: Escape all double quotes
    wchar_t *escaped_str = replace_smart((wchar_t *)input, L"\"", L"\\\"");
    if (escaped_str == NULL) {
        // If replacement failed, copy input as-is
        wcsncpy_s(output, output_size, input, _TRUNCATE);
        return;
    }

    // Step 2: Remove single quotes immediately after escaped double quotes (\"')
    wchar_t *temp1 = replace_smart(escaped_str, L"\\\"'", L"\\\"");
    HeapFree(GetProcessHeap(), 0, escaped_str);
    if (temp1 == NULL) {
        // If replacement failed, copy escaped_str as-is
        wcsncpy_s(output, output_size, L"", _TRUNCATE); // Empty string
        return;
    }

    // Step 3: Remove single quotes immediately before escaped double quotes ('\")
    wchar_t *temp2 = replace_smart(temp1, L"'\\\"", L"\\\"");
    HeapFree(GetProcessHeap(), 0, temp1);
    if (temp2 == NULL) {
        // If replacement failed, copy temp1 as-is
        wcsncpy_s(output, output_size, temp1, _TRUNCATE);
        return;
    }

    // Step 4: Copy the final string to output
    wcsncpy_s(output, output_size, temp2, _TRUNCATE);
    HeapFree(GetProcessHeap(), 0, temp2);
}

// Function to perform command/string replacements based on environment variables
void apply_environment_replacements(LPWSTR cmdline) {
    WCHAR bufW[MAX_PATH];
    if (GetEnvironmentVariableW(L"PSHACKS", bufW, MAX_PATH)) {
        WCHAR buf_fromW[MAX_PATH];
        WCHAR buf_toW[MAX_PATH];
        WCHAR *buf_replacedW = NULL;

        if (GetEnvironmentVariableW(L"PS_FROM", buf_fromW, MAX_PATH) &&
            GetEnvironmentVariableW(L"PS_TO", buf_toW, MAX_PATH)) {
            wchar_t *bufferA, *bufferB = 0;

            wchar_t *tokenA = wcstok_s(buf_fromW, L"¶", &bufferA);
            wchar_t *tokenB = wcstok_s(buf_toW, L"¶", &bufferB);

            while (tokenA && tokenB) {
                buf_replacedW = replace_smart(cmdline, tokenA, tokenB);
                if (buf_replacedW == NULL) {
                    // Handle memory allocation failure
                    fwprintf(stderr, L"Failed to replace string in cmdline.\n");
                    return;
                }
                wcscpy_s(cmdline, MAX_PATH, buf_replacedW);
                HeapFree(GetProcessHeap(), 0, buf_replacedW);

                tokenA = wcstok_s(NULL, L"¶", &bufferA);
                tokenB = wcstok_s(NULL, L"¶", &bufferB);
            }
        }
    }
}

#ifdef ENABLE_DEBUG_LOG
// Function to log the Command Line to a File
static void LogCommandLine(LPCWSTR cmdline) {
    // Open the log file in append mode with UTF-8 encoding
    FILE *logFile = _wfopen(L"C:\\debug.log", L"a, ccs=UTF-8");
    if (logFile) {
        // Write the timestamp and command line to the log file
        fwprintf(logFile, L"[%ls] Command line: %ls\n", __DATE__ L" " __TIME__, cmdline);

        // Close the log file
        fclose(logFile);
    } else {
        // If the log file couldn't be opened, optionally handle the error
        // For example, write to stderr (optional)
        fwprintf(stderr, L"Failed to open log file for debugging.\n");
    }
}

// Define a macro for logging
#define LOG_CMDLINE(cmdline) LogCommandLine(cmdline)
#else
// Define a no-op macro when debugging is disabled
#define LOG_CMDLINE(cmdline) ((void)0)
#endif

int wmain() {
    wchar_t cmdlineW[8192] = L"", pwsh_pathW[MAX_PATH] = L"", pwsh_exeW[MAX_PATH] = L"";
    DWORD exitcode;
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi = {0};

    // Initialize STARTUPINFO structure
    si.cb = sizeof(STARTUPINFOW);

    // Get the correct Program Files path based on the process architecture
    get_program_files_path(pwsh_pathW, MAX_PATH);

    // Get the executable name (pwsh.exe)
    wcscpy_s(pwsh_exeW, MAX_PATH, PathFindFileNameW(pwsh_pathW));

    // Set environment variable to disable color rendering output
    _wputenv_s(L"NO_COLOR", L"1");

    // Set environment variable to enable debugging on the powershell side
    #ifdef ENABLE_DEBUG_LOG
    _wputenv_s(L"LOG_DEBUG", L"1");
    #endif

    // Get the full command line
    LPCWSTR cmdline_full = GetCommandLineW();

    // Skip the program name
    LPCWSTR argsW = SkipProgramName(cmdline_full);

    // Build the command line to pass to pwsh.exe
    wcscpy_s(cmdlineW, sizeof(cmdlineW) / sizeof(wchar_t), L"\"");
    wcscat_s(cmdlineW, sizeof(cmdlineW) / sizeof(wchar_t), pwsh_exeW);
    wcscat_s(cmdlineW, sizeof(cmdlineW) / sizeof(wchar_t), L"\"");

    // Check if we need to add '-c' option
    if (*argsW) {
        if (should_add_c_option(argsW)) {
            wcscat_s(cmdlineW, sizeof(cmdlineW) / sizeof(wchar_t), L" -c");

            // Enclose the command in double quotes
            wcscat_s(cmdlineW, sizeof(cmdlineW) / sizeof(wchar_t), L" \"");

            // Escape any double quotes in argsW and fix single quotes
            wchar_t escaped_args[8192];
            escape_and_fix_quotes(argsW, escaped_args, sizeof(escaped_args) / sizeof(wchar_t));

            wcscat_s(cmdlineW, sizeof(cmdlineW) / sizeof(wchar_t), escaped_args);
            wcscat_s(cmdlineW, sizeof(cmdlineW) / sizeof(wchar_t), L"\"");
        } else {
            // Append the rest of the arguments as is
            wcscat_s(cmdlineW, sizeof(cmdlineW) / sizeof(wchar_t), L" ");
            wcscat_s(cmdlineW, sizeof(cmdlineW) / sizeof(wchar_t), argsW);
        }
    } else if (is_input_from_pipeline()) {
        // If there's no arguments but input is from pipeline, we need to read from stdin
        wcscat_s(cmdlineW, sizeof(cmdlineW) / sizeof(wchar_t), L" -c -");
    }

    // Apply environment replacements if any
    apply_environment_replacements(cmdlineW);

    // **Debugging: Log the Command Line**
    LOG_CMDLINE(cmdlineW);

    // Execute the command through pwsh.exe
    if (!CreateProcessW(pwsh_pathW, cmdlineW, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        fwprintf(stderr, L"Failed to create process. Error code: %lu\n", GetLastError());
        return 1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exitcode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitcode;
}
