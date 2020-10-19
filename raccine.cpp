// Raccine 
// A Simple Ransomware Vaccine
// https://github.com/Neo23x0/Raccine
//
// Florian Roth, Ollie Whitehouse, Branislav Dalic, John Lambert
// with help of Hilko Bengen

#include <wchar.h>
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <psapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <strsafe.h>
#include <shellapi.h>

#include "resource.h"

#define BIT_MASK(a, b) (((unsigned) -1 >> (31 - (b))) & ~((1U << (a)) - 1))

#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
// Version
#define VERSION L"0.10.3"

// Log Config and Flags
int g_fLogSettings = 0;
BOOL g_fLogOnly = FALSE;
#define RACCINE_REG_CONFIG  L"SOFTWARE\\Raccine"
#define RACCINE_REG_POICY_CONFIG  L"SOFTWARE\\Policies\\Raccine"
#define MAX_MESSAGE 1000
#define RACCINE_DEFAULT_EVENTID  1
#define RACCINE_EVENTID_MALICIOUS_ACTIVITY  2

#define RACCINE_LOG_TO_CONSOLE  0x1
#define RACCINE_LOG_TO_EVENTLOG 0x2

std::wstring sListLogs(L"");

// UX defines and globals
#define RACCINE_TOOLTIP L"Raccine Notification"
#define APPWM_ICONNOTIFY (WM_APP + 1)
#define APPWM_ALERT (WM_APP + 2)

wchar_t const szWindowClass[] = L"RaccineNotificationIcon";

HWND g_Hwnd = 0;
HINSTANCE g_hInst = 0;
HBITMAP g_hBitmap = NULL;
#define ID_EDITCHILD 100

#define DEFAULT_HEIGHT 200
#define DEFAULT_WIDTH  400


//void RegisterNotificationIcon(LPWSTR szInfo, LPWSTR szInfoTitle)
//{
//    HWND hwnd = g_Hwnd;
//    HICON hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDCICON));
//
//    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
//    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
//
//    //Notification
//    NOTIFYICONDATA nid = {};
//    nid.cbSize = sizeof(nid);
//    nid.hWnd = g_Hwnd;
//    nid.uID = 1;
//    nid.uFlags = NIF_ICON | NIF_MESSAGE;
//    nid.uCallbackMessage = APPWM_ICONNOTIFY;
//    nid.hIcon = hIcon;
//    size_t cchInfo = 0;
//    size_t cchInfoTitle = 0;
//    if (FAILED(StringCchLength(szInfo, 256, &cchInfo)))
//        return;
//
//    if (FAILED(StringCchLength(szInfoTitle, 256, &cchInfoTitle)))
//        return;
//
//    StringCchCopyW(nid.szInfo, cchInfo, szInfo);
//    StringCchCopyW(nid.szInfoTitle, cchInfoTitle, szInfoTitle);
//    // This text will be shown as the icon's tooltip.
//    StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), RACCINE_TOOLTIP);
//
//    // Show the notification.
//    Shell_NotifyIcon(NIM_ADD, &nid);
//}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    HWND static hwndEdit = 0;
    BITMAP static bitmap = { 0 };

    switch (uMsg)
    {
        case WM_CREATE:
        {
            //SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP);
            g_hBitmap = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDCLOGO));

            if (!g_hBitmap)
                return 0;
            GetObject(g_hBitmap, sizeof(bitmap), &bitmap);

            RECT rc = { 0 };
            GetWindowRect(hwnd, &rc);
            int xPos = (GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2;
            int yPos = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2;

            hwndEdit = CreateWindowEx(
                0, L"EDIT",   // predefined class 
                NULL,         // no window title 
                WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
                0, 0, 0, 0,   // set size in WM_SIZE message 
                hwnd,         // parent window 
                (HMENU)ID_EDITCHILD,   // edit control ID 
                (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
                NULL);        // pointer not needed 

            int nHeight = -MulDiv(10, GetDeviceCaps(GetDC(hwnd), LOGPIXELSY), 72);

            HFONT hFont = CreateFont(nHeight, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
                OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE, TEXT("Consolas"));
            SendMessage(hwndEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

            SetWindowPos(hwnd, 0, xPos, yPos, bitmap.bmWidth + DEFAULT_WIDTH, bitmap.bmHeight + DEFAULT_HEIGHT, SWP_NOZORDER);

            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT     ps = { 0 };
            HDC             hdc = NULL;
            HDC             hdcMem = NULL;
            HGDIOBJ         oldBitmap = NULL;

            hdc = BeginPaint(hwnd, &ps);

            hdcMem = CreateCompatibleDC(hdc);
            oldBitmap = SelectObject(hdcMem, g_hBitmap);

            BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, oldBitmap);
            DeleteDC(hdcMem);

            EndPaint(hwnd, &ps);
            break;
        }
        case WM_APPCOMMAND:
        {
            if (lParam == APPWM_ALERT)
            {
                SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)sListLogs.c_str());
                return 0;
            }
        }
        case WM_SETFOCUS:
        {
            SetFocus(hwndEdit);
            return 0;
        }
        case WM_SIZE:
        {
            // Make the edit control the size of the window's client area. 

            MoveWindow(hwndEdit,
                0, bitmap.bmHeight,    // starting x- and y-coordinates 
                LOWORD(lParam),        // width of client area 
                HIWORD(lParam),        // height of client area 
                TRUE);                 // repaint window 
            return 0;
        }
        case APPWM_ICONNOTIFY:
        {
            switch (lParam)
            {
            case WM_LBUTTONUP:
                //...
                break;
            case WM_RBUTTONUP:
                //...
                break;
            }
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/// This function will optionally log messages to the eventlog
void WriteEventLogEntryWithId(LPWSTR  pszMessage, DWORD dwEventId)
{
    if (BIT_MASK(g_fLogSettings, RACCINE_LOG_TO_EVENTLOG))
    {
        HANDLE hEventSource = NULL;
        LPCWSTR lpszStrings[2] = { NULL, NULL };

        hEventSource = RegisterEventSource(NULL, L"Raccine");
        if (hEventSource)
        {
            lpszStrings[0] = pszMessage;
            lpszStrings[1] = NULL;


            ReportEvent(hEventSource,  // Event log handle
                EVENTLOG_INFORMATION_TYPE,                 // Event type
                0,                     // Event category
                dwEventId,                     // Event identifier
                NULL,                  // No security identifier
                1,  // Size of lpszStrings array
                0,                     // No binary data
                lpszStrings,           // Array of strings
                NULL                   // No binary data
            );

            DeregisterEventSource(hEventSource);
        }
    }
    // always print the message to the console
    wprintf(pszMessage);
}

void WriteEventLogEntry(LPWSTR  pszMessage)
{
    WriteEventLogEntryWithId(pszMessage, RACCINE_DEFAULT_EVENTID);
}

// Get Parent Process ID
DWORD getParentPid(DWORD pid) {
    PROCESSENTRY32 pe32;
    HANDLE hSnapshot;
    DWORD ppid = 0;
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE) {
        goto out;
    }
    ZeroMemory(&pe32, sizeof(pe32));
    pe32.dwSize = sizeof(pe32);
    if (!Process32First(hSnapshot, &pe32)) {
        goto out;
    }
    do {
        if (pe32.th32ProcessID == pid) {
            ppid = pe32.th32ParentProcessID;
            break;
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
out:
    return ppid;
}

// Get integrity level of process
DWORD getIntegrityLevel(HANDLE hProcess) {

    HANDLE hToken = INVALID_HANDLE_VALUE;
    DWORD dwIntegrityLevel = 0;
    PTOKEN_MANDATORY_LABEL pTIL;
    DWORD dwLengthNeeded = sizeof(pTIL);

    if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
        return 0;
    
    GetTokenInformation(hToken, TokenIntegrityLevel, NULL, 0, &dwLengthNeeded);
    pTIL = (PTOKEN_MANDATORY_LABEL)LocalAlloc(0, dwLengthNeeded);
    if (!pTIL) {
        return 0;
    }

    if (GetTokenInformation(hToken, TokenIntegrityLevel,
        pTIL, dwLengthNeeded, &dwLengthNeeded)) {
        dwIntegrityLevel = *GetSidSubAuthority(pTIL->Label.Sid,
            (DWORD)(UCHAR)(*GetSidSubAuthorityCount(pTIL->Label.Sid) - 1));

        LocalFree(pTIL);

        if (dwIntegrityLevel == SECURITY_MANDATORY_LOW_RID) {
            // Low Integrity
            return 1;
        }
        else if (dwIntegrityLevel >= SECURITY_MANDATORY_MEDIUM_RID &&
            dwIntegrityLevel < SECURITY_MANDATORY_HIGH_RID) {
            // Medium Integrity
            return 2;
        }
        else if (dwIntegrityLevel >= SECURITY_MANDATORY_HIGH_RID &&
            dwIntegrityLevel < SECURITY_MANDATORY_SYSTEM_RID) {
            // High Integrity
            return 3;
        }
        else if (dwIntegrityLevel >= SECURITY_MANDATORY_SYSTEM_RID) {
            // System Integrity
            return 4;
        }
        else {
            return 0;
        }
    }
    else {
        LocalFree(pTIL);
        return 0;
    }
    return 0;
}

// Check if process is in allowed list
BOOL isallowlisted(DWORD pid) {
    WCHAR allowlist[3][MAX_PATH] = { L"wininit.exe", L"winlogon.exe", L"explorer.exe" };
    PROCESSENTRY32 pe32 = { 0 };
    HANDLE hSnapshot;
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE) {
        goto out;
    }

    ZeroMemory(&pe32, sizeof(pe32));
    pe32.dwSize = sizeof(pe32);

    if (!Process32First(hSnapshot, &pe32)) {
        goto out;
    }

    do {
        if (pe32.th32ProcessID == pid) {
            for (uint8_t i = 0; i < ARRAYSIZE(allowlist); i++) {

                if (_wcsicmp((wchar_t*)pe32.szExeFile, allowlist[i]) == 0) {

                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);

                    if (hProcess != NULL) {
                        wchar_t filePath[MAX_PATH] = { 0 };
                        if (GetModuleFileNameEx(hProcess, NULL, filePath, MAX_PATH)) {
                            DWORD dwInLevel = getIntegrityLevel(hProcess);

                            // Are they in the Windows directory?
                            if (_wcsnicmp(filePath, L"C:\\Windows\\System32\\", wcslen(L"C:\\Windows\\System32\\")) == 0) {

                                // Is the process running as SYSTEM
                                if (getIntegrityLevel(hProcess) == 4) {
                                    CloseHandle(hProcess);
                                    CloseHandle(hSnapshot);
                                    return TRUE;
                                }
                            }

                            // Are you explorer running in the Windows dir
                            if (_wcsnicmp(filePath, L"C:\\Windows\\Explorer.exe", wcslen(L"C:\\Windows\\Explorer.exe")) == 0) {

                                // Is the process running as MEDIUM (which Explorer does)
                                if (getIntegrityLevel(hProcess) == 2) {
                                    CloseHandle(hProcess);
                                    CloseHandle(hSnapshot);
                                    return TRUE;
                                }
                            }
                        }
                        else {
                            CloseHandle(hProcess);
                        }
                    }
                } // _wcsicmp
            }
            break;
        }
    } while (Process32Next(hSnapshot, &pe32));

out:
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        CloseHandle(hSnapshot);
    }
    return FALSE;
}

// Kill a process
BOOL killProcess(DWORD dwProcessId, UINT uExitCode) {
    DWORD dwDesiredAccess = PROCESS_TERMINATE;
    BOOL  bInheritHandle = FALSE;
    HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
    if (hProcess == NULL)
        return FALSE;

    BOOL result = TerminateProcess(hProcess, uExitCode);
    CloseHandle(hProcess);
    return result;
}

// Get timestamp
std::string getTimeStamp() {
    struct tm buf;
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::hours(24));
    localtime_s(&buf, &time);
    std::stringstream ss;
    ss << std::put_time(&buf, "%F %T");
    auto timestamp = ss.str();
    return timestamp;
}

// Fomat a log lines
std::wstring logFormat(const std::wstring cmdLine, const std::wstring comment = L"done") {
    std::string timeString = getTimeStamp();
    std::wstring timeStringW(timeString.begin(), timeString.end());
    std::wstring logLine = timeStringW + L" DETECTED_CMD: '" + cmdLine + L" COMMENT: " + comment + L"\n";
    return logLine;
}

// Format the activity log lines
std::wstring logFormatAction(int pid, const std::wstring cmdLine, const std::wstring comment = L"done") {
    std::string timeString = getTimeStamp();
    std::wstring timeStringW(timeString.begin(), timeString.end());
    std::wstring logLine = timeStringW + L" DETECTED_CMD: '" + cmdLine + L"' PID: " + std::to_wstring(pid) + L" ACTION: " + comment + L"\n";
    return logLine;
}

// Log to file
void logSend(const std::wstring logStr) {
    static FILE* logFile = 0;
    if (logFile == 0) 
    {
        errno_t err = _wfopen_s(&logFile, L"C:\\ProgramData\\Raccine_log.txt", L"at");
        if (err != 0) 
            err = _wfopen_s(&logFile, L"C:\\ProgramData\\Raccine_log.txt", L"wt");
            
        if (err != 0) {
            wprintf(L"\nCan not open C:\\ProgramData\\Raccine_log.txt for writing.\n");
            return;   // bail out if we can't log
        }
    }
    //transform(logStr.begin(), logStr.end(), logStr.begin(), ::tolower);
    if (logFile != 0)
    {
        fwprintf(logFile, L"%s", logStr.c_str());
        fflush(logFile);
        fclose(logFile);
        logFile = 0;
    }
}

//
//  Query for config in HKLM and HKLM\Software\Policies override by GPO
//
void InitializeLoggingSettings()
{
    // Registry Settings
    // Query for logging level. A value of 1 or more indicates to log key events to the event log
    // Query for logging only mode. A value of 1 or more indicates to suppress process kills

    const wchar_t* LoggingKeys[] = { RACCINE_REG_CONFIG , RACCINE_REG_POICY_CONFIG };

    HKEY hKey = NULL;
    for (int i = 0; i < ARRAYSIZE(LoggingKeys); i++)
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, LoggingKeys[i], 0, KEY_READ, &hKey))
        {
            // Log Level
            DWORD dwLoggingLevel = 0;
            DWORD cbData = sizeof(dwLoggingLevel);
            if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"Logging", NULL, NULL, (LPBYTE)&dwLoggingLevel, &cbData))
            {
                g_fLogSettings = dwLoggingLevel;  // This can be a bitmask
            }
            // Log Only
            DWORD dwLoggingOnly = 0;
            DWORD cbDataLO = sizeof(dwLoggingOnly);
            if (ERROR_SUCCESS == RegQueryValueExW(hKey, L"LogOnly", NULL, NULL, (LPBYTE)&dwLoggingOnly, &cbDataLO))
            {
                if (dwLoggingOnly > 0)
                {
                    g_fLogOnly = TRUE;
                }
            }
            RegCloseKey(hKey);
        }
    }
}

void RaccineAlert()
{
    SendMessage(g_Hwnd, WM_APPCOMMAND, 0, APPWM_ALERT);
    ShowWindow(g_Hwnd, SW_SHOW);
}

DWORD WINAPI WorkerThread(LPVOID lpParameter)
{
    DWORD pids[1024] = { 0 };
    uint8_t c = 0;
    DWORD pid = GetCurrentProcessId();
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    setlocale(LC_ALL, "");

    /*
    if (BIT_MASK(g_fLogSettings, RACCINE_LOG_TO_CONSOLE))
    {
        // create the console
        if (AllocConsole())
        {
            FILE* pCout;
            freopen_s(&pCout, "CONOUT$", "w", stdout);
            SetConsoleTitle(L"Racine Logging Console");
        }
    }
    */

    // Block marker
    bool bBlock = false;

    // Main programs to monitor
    bool bVssadmin = false;
    bool bWmic = false;
    bool bWbadmin = false;
    bool bcdEdit = false;
    bool bPowerShell = false;
    bool bDiskShadow = false;

    // Command line params
    bool bDelete = false;
    bool bShadows = false;
    bool bResize = false;
    bool bShadowStorage = false;
    bool bShadowCopy = false;
    bool bCatalog = false;
    bool bQuiet = false;
    bool bRecoveryEnabled = false;
    bool bIgnoreallFailures = false;
    bool bwin32ShadowCopy = false;
    bool bEncodedCommand = false;

    // Encoded Command List (Base64)
    WCHAR encodedCommands[7][9] = { L"JAB", L"SQBFAF", L"SQBuAH", L"SUVYI", L"cwBhA", L"aWV4I", L"aQBlAHgA" };

    // Log
    std::wstring sCommandLine = L"";
    WCHAR wMessage[MAX_MESSAGE] = { 0 };

    // Append all original command line parameters to a string for later log messages
    for (int i = 1; i < argc; i++) sCommandLine.append(std::wstring(argv[i]).append(L" "));

    if (argc > 1)
    {
        // Check for invoked program 
        if ((_wcsicmp(L"vssadmin.exe", argv[1]) == 0) ||
            (_wcsicmp(L"vssadmin", argv[1]) == 0)) {
            bVssadmin = true;
        }
        else if ((_wcsicmp(L"wmic.exe", argv[1]) == 0) ||
            (_wcsicmp(L"wmic", argv[1]) == 0)) {
            bWmic = true;
        }
        else if ((_wcsicmp(L"wbadmin.exe", argv[1]) == 0) ||
            (_wcsicmp(L"wbadmin", argv[1]) == 0)) {
            bWbadmin = true;
        }
        else if ((_wcsicmp(L"bcdedit.exe", argv[1]) == 0) ||
            (_wcsicmp(L"bcdedit", argv[1]) == 0)) {
            bcdEdit = true;
        }
        else if ((_wcsicmp(L"powershell.exe", argv[1]) == 0) ||
            (_wcsicmp(L"powershell", argv[1]) == 0)) {
            bPowerShell = true;
        }
        else if ((_wcsicmp(L"diskshadow.exe", argv[1]) == 0) ||
            (_wcsicmp(L"diskshadow", argv[1]) == 0)) {
            bDiskShadow = true;
        }
    }

    // Check for keywords in command line parameters
    for (int iCount = 1; iCount < argc; iCount++) {

        // Convert wchar to wide string so we can perform contains/find command
        wchar_t* convertedCh = argv[iCount];
        wchar_t* convertedChOrig = argv[iCount];    // original parameter (no tolower)
        wchar_t* convertedChPrev = argv[iCount - 1];  // previous parameter
        // Convert them to wide strings
        std::wstring convertedArg(convertedCh);
        std::wstring convertedArgOrig(convertedChOrig);
        std::wstring convertedArgPrev(convertedChPrev);

        // Convert args to lowercase for case-insensitive comparisons
        transform(convertedArg.begin(), convertedArg.end(), convertedArg.begin(), ::tolower);
        transform(convertedArgPrev.begin(), convertedArgPrev.end(), convertedArgPrev.begin(), ::tolower);

        // Simple flag checks
        if (_wcsicmp(L"delete", argv[iCount]) == 0) {
            bDelete = true;
        }
        else if (_wcsicmp(L"shadows", argv[iCount]) == 0) {
            bShadows = true;
        }
        else if (_wcsicmp(L"shadowstorage", argv[iCount]) == 0) {
            bShadowStorage = true;
        }
        else if (_wcsicmp(L"resize", argv[iCount]) == 0) {
            bResize = true;
        }
        else if (_wcsicmp(L"shadowcopy", argv[iCount]) == 0) {
            bShadowCopy = true;
        }
        else if (_wcsicmp(L"catalog", argv[iCount]) == 0) {
            bCatalog = true;
        }
        else if (_wcsicmp(L"-quiet", argv[iCount]) == 0 || _wcsicmp(L"/quiet", argv[iCount]) == 0) {
            bQuiet = true;
        }
        else if (_wcsicmp(L"recoveryenabled", argv[iCount]) == 0) {
            bRecoveryEnabled = true;
        }
        else if (_wcsicmp(L"ignoreallfailures", argv[iCount]) == 0) {
            bIgnoreallFailures = true;
        }
        else if (convertedArg.find(L"win32_shadowcopy") != std::string::npos) {
            bwin32ShadowCopy = true;
        }
        // Special comparison of current argument with previous argument
        // allows to check for e.g. -encodedCommand JABbaTheHuttandotherBase64characters
        else if (convertedArgPrev.find(L"-e") != std::string::npos || convertedArgPrev.find(L"/e") != std::string::npos) {
            for (uint8_t i = 0; i < ARRAYSIZE(encodedCommands); i++) {
                if (convertedArgOrig.find(encodedCommands[i]) != std::string::npos) {
                    bEncodedCommand = true;
                }
            }
        }
    }

    InitializeLoggingSettings();

    // Check all combinations (our blocklist)
    if ((bVssadmin && bDelete && bShadows) ||             // vssadmin.exe
        (bVssadmin && bDelete && bShadowStorage) ||      // vssadmin.exe
        (bVssadmin && bResize && bShadowStorage) ||      // vssadmin.exe
        (bWmic && bDelete && bShadowCopy) ||             // wmic.exe
        (bWbadmin && bDelete && bCatalog && bQuiet) || 	 // wbadmin.exe 
        (bcdEdit && bIgnoreallFailures) ||               // bcdedit.exe
        (bcdEdit && bRecoveryEnabled) ||                 // bcdedit.exe
        (bDiskShadow && bDelete && bShadows) ||          // diskshadow.exe
        (bPowerShell && bwin32ShadowCopy) ||             // powershell.exe
        (bPowerShell && bEncodedCommand) ||              // powershell.exe
        (bDiskShadow && bDelete && bShadows)) {          // diskshadow.exe

        // Activate blocking
        bBlock = TRUE;
    }

    // If activity that should be block has been registered (always log)
    if (bBlock) {
        // Log to the windows Eventlog
        LPCWSTR lpMessage = sCommandLine.c_str();
        if (!g_fLogOnly) {
            // Eventlog
            StringCchPrintf(wMessage, ARRAYSIZE(wMessage), L"Raccine detected malicious activity:\n%s\n", lpMessage);
            // Log to the text log file
            sListLogs.append(logFormat(sCommandLine, L"Raccine detected malicious activity"));

        }
        else {
            // Eventlog
            StringCchPrintf(wMessage, ARRAYSIZE(wMessage), L"Raccine detected malicious activity:\n%s\n(simulation mode)", lpMessage);
            // Log to the text log file
            sListLogs.append(logFormat(sCommandLine, L"Raccine detected malicious activity (simulation mode)"));
        }
        RaccineAlert();
        // add the notification icon
        //RegisterNotificationIcon(wMessage, (LPWSTR)L"Raccine Alert");
        WriteEventLogEntryWithId((LPWSTR)wMessage, RACCINE_EVENTID_MALICIOUS_ACTIVITY);
    }

    // If block and not simulation mode
    if (bBlock && !g_fLogOnly) {
        // Collect PIDs to kill
        while (c < 1024) {
            pid = getParentPid(pid);
            if (pid == 0) {
                break;
            }
            if (!isallowlisted(pid)) {
                wprintf(L"\nCollecting PID %d for a kill\n", pid);
                pids[c] = pid;
                c++;
            }
            else {
                wprintf(L"\nProcess with PID %d is on allowlist\n", pid);
                sListLogs.append(logFormatAction(pid, sCommandLine, L"Whitelisted"));
            }
        }

        // Loop over collected PIDs and try to kill the processes
        for (uint8_t i = c; i > 0; --i) {
            // If no simulation flag is set
            if (!g_fLogOnly) {
                // Kill
                wprintf(L"Kill PID %d\n", pids[i - 1]);
                killProcess(pids[i - 1], 1);
                sListLogs.append(logFormatAction(pids[i - 1], sCommandLine, L"Terminated"));
            }
            else {
                // Simulated kill
                wprintf(L"Simulated Kill PID %d\n", pids[i - 1]);
                sListLogs.append(logFormatAction(pids[i - 1], sCommandLine, L"Terminated (Simulated)"));
            }
        }
        // Finish message
        wprintf(L"\nRaccine v%s finished\n", VERSION);
        Sleep(5000);
    }

    // Otherwise launch the process with its original parameters
    // Conditions:
    // a.) not block or
    // b.) simulation mode
    if (!bBlock || g_fLogOnly) {
        DEBUG_EVENT debugEvent = { 0 };
        std::wstring sCommandLineStr = L"";

        for (int i = 1; i < argc; i++) sCommandLineStr.append(std::wstring(argv[i]).append(L" "));

        STARTUPINFO info = { sizeof(info) };
        PROCESS_INFORMATION processInfo = { 0 };

        if (CreateProcess(NULL, (LPWSTR)sCommandLineStr.c_str(), NULL, NULL, TRUE, DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS, NULL, NULL, &info, &processInfo))
        {
            DebugActiveProcessStop(processInfo.dwProcessId);
            WaitForSingleObject(processInfo.hProcess, INFINITE);
            CloseHandle(processInfo.hProcess);
            CloseHandle(processInfo.hThread);
        }
    }

    // Log events
    logSend(sListLogs);

    if (argv)
    {
        LocalFree(argv);
        argv = NULL;
    }

    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
    g_hInst = hInstance;

    WNDCLASSEX wcex = { sizeof(wcex) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDCICON));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    RegisterClassEx(&wcex);

    
    HWND hwnd = CreateWindow(szWindowClass, RACCINE_TOOLTIP, WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT, NULL, NULL, hInstance, NULL);

    g_Hwnd = hwnd;
    SendMessage(g_Hwnd, WM_APPCOMMAND, 0, APPWM_ALERT);

    DWORD dwThreadId = 0;
    HANDLE hWorkerThread = CreateThread(
        NULL,
        0,
        WorkerThread,
        NULL,
        0,
        &dwThreadId);

    if (hWorkerThread == NULL)
        goto cleanup;

    if (hwnd)
    {
        ShowWindow(hwnd, SW_HIDE);

        // Main message loop:
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    WaitForSingleObject(hWorkerThread, INFINITE);

cleanup:

    if (hWorkerThread)
        CloseHandle(hWorkerThread);
    return 0;
}
