// Raccine 
// A Simple Ransomware Vaccine
// https://github.com/Neo23x0/Raccine
//
// Florian Roth, Ollie Whitehouse, Branislav Djalic, John Lambert
// with help of Hilko Bengen

#include "source/RaccineLib/Raccine.h"

#include <Shlwapi.h>

#include "source/RaccineLib/HandleWrapper.h"
#include "source/RaccineLib/RaccineConfig.h"

int wmain(int argc, WCHAR* argv[])
{
    setlocale(LC_ALL, "");

    std::vector<std::wstring> command_line;
    std::wstring sCommandLine;

    // Append all original command line parameters to a string for later log messages
    for (int i = 1; i < argc; i++) {
        sCommandLine.append(std::wstring(argv[i]).append(L" "));
        command_line.emplace_back(argv[i]);
    }

    const RaccineConfig configuration;

    // Launch the new child in a suspended state (CREATE_SUSPENDED)
    // this will allow yara rules to run against this process
    // if we should block, we will terminate it later
    std::wstring sCommandLineStr = sCommandLine;
    if (bPowerShellWorkaround) {
        sCommandLineStr = std::wstring(L"powershell.exe ").append(sCommandLine);
    }

    createChildProcessWithDebugger(sCommandLineStr, CREATE_SUSPENDED, &dwChildPid, &hProcess, &hThread);


    DWORD dwParentPid = utils::getParentPid(GetCurrentProcessId());
    std::wstring szYaraOutput;
    const bool fYaraRuleMatched = EvaluateYaraRules(configuration.yara_rules_directory(), 
                                                    sCommandLine, 
                                                    szYaraOutput, 
                                                    dwChildPid, 
                                                    dwParentPid);

    if (fYaraRuleMatched) {
        bBlock = true;
    }

    std::wstring sListLogs;

    // If activity that should be block has been registered (always log)
    if (bBlock) {
        std::wstring message;
        // Log to the windows Eventlog
        if (!configuration.log_only()) {
            // Eventlog
            message = L"Raccine detected malicious activity:\n" + sCommandLine + L"\n";
            // Log to the text log file
            sListLogs.append(logFormat(sCommandLine, L"Raccine detected malicious activity"));
        } else {
            // Eventlog
            message = L"Raccine detected malicious activity:\n" + sCommandLine + L"\n(simulation mode)";
            // Log to the text log file
            sListLogs.append(logFormat(sCommandLine, L"Raccine detected malicious activity (simulation mode)"));
        }

        WriteEventLogEntryWithId(message, RACCINE_EVENTID_MALICIOUS_ACTIVITY);


        // YARA Matches Detected
        if (fYaraRuleMatched && !szYaraOutput.empty()) {
            message += L"\r\nYara matches:\r\n" + szYaraOutput;
            WriteEventLogEntryWithId(message, RACCINE_EVENTID_MALICIOUS_ACTIVITY);
            sListLogs.append(logFormatLine(szYaraOutput));
        }

        // signal Event for UI to know an alert happened.  If no UI is running, this has no effect.
        if (configuration.show_gui()) {
            trigger_gui_event();
        }
    }

    // If block and not simulation mode
    if (bBlock && !configuration.log_only()) {
        find_and_kill_processes(configuration.log_only(), sCommandLine, sListLogs);
    }


    // if we're in simulation mode or we didn't need to block the process, let it run
    if (g_fLogOnly || !bBlock)
    {
        if (hThread != INVALID_HANDLE_VALUE && hProcess != INVALID_HANDLE_VALUE)
        {

            ResumeThread(hThread);
            WaitForSingleObject(hProcess, INFINITE);
            CloseHandle(hThread);
            CloseHandle(hProcess);
        }
    }
    else
    {
        if (bBlock)
        {
           utils::killProcess(dwChildPid, 1);
            CloseHandle(hThread);
            CloseHandle(hProcess);
        }
    }

    // Log events
    logSend(sListLogs);

    return 0;
}
