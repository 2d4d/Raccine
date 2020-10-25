#pragma once

// Raccine 
// A Simple Ransomware Vaccine
// https://github.com/Neo23x0/Raccine
//
// Florian Roth, Ollie Whitehouse, Branislav Djalic, John Lambert
// with help of Hilko Bengen

#include <set>
#include <Windows.h>
#include <string>
#include "YaraRuleRunner.h"
#include "utils.h"


// Version
#define VERSION "1.0.4 BETA"

// Log Config and Flags
#define RACCINE_DEFAULT_EVENTID  1
#define RACCINE_EVENTID_MALICIOUS_ACTIVITY  2

#define RACCINE_DATA_DIRECTORY  L"%PROGRAMDATA%\\Raccine"
#define RACCINE_YARA_DIRECTORY  L"%PROGRAMDATA%\\Raccine\\yara"
#define RACCINE_PROGRAM_DIRECTORY  L"%PROGRAMFILES%\\Raccine"
#define RACCINE_USER_CONTEXT_DIRECTORY  L"%TEMP%\\RaccineUserContext"


enum class Integrity
{
    Error = 0, // Indicates integrity level could not be found
    Low = 1,
    Medium = 2,
    High = 3,
    System = 4,


/// <summary>
/// Evaluate a set of yara rules on a command line
/// </summary>
/// <param name="yara_rules_directory">The directory containing the yara rules</param>
/// <param name="lpCommandLine">The command line to test</param>
/// <param name="outYaraOutput">if not empty, an output string containing match results is written to this parameter.</param>
/// <returns>TRUE if at least one match result was found</returns>
bool EvaluateYaraRules(const std::wstring& yara_rules_directory,
                       const std::wstring& lpCommandLine, 
                       std::wstring& outYaraOutput, 
                       DWORD dwChildPid, 
                       DWORD dwParentPid);

/// This function will optionally log messages to the eventlog
void WriteEventLogEntryWithId(const std::wstring& pszMessage, DWORD dwEventId);

void WriteEventLogEntry(const std::wstring& pszMessage);

bool is_malicious_command_line(const std::vector<std::wstring>& command_line);

bool does_command_line_contain_base64(const std::vector<std::wstring>& command_line);

bool needs_powershell_workaround(const std::wstring& command_line);

void trigger_gui_event();

// Check if process is in allowed list
bool isAllowListed(DWORD pid);


// Get timestamp
std::string getTimeStamp();

// Format a log lines
std::wstring logFormat(const std::wstring& cmdLine, const std::wstring& comment = L"done");

std::wstring logFormatLine(const std::wstring& line = L"");

// Format the activity log lines
std::wstring logFormatAction(DWORD pid, const std::wstring& imageName, const std::wstring& cmdLine, const std::wstring& comment = L"done");

// Log to file
void logSend(const std::wstring& logStr);

void createChildProcessWithDebugger(std::wstring command_line, DWORD dwAdditionalCreateParams, PDWORD pdwChildPid, PHANDLE phProcess, PHANDLE phThread);

// Find all parent processes
std::set<DWORD> find_processes_to_kill(const std::wstring& sCommandLine, std::wstring& sListLogs);

// Find all parent processes and kill them
void find_and_kill_processes(bool log_only, 
                             const std::wstring& sCommandLine, 
                             std::wstring& sListLogs);

void CreateContextFileForProgram(DWORD pid, DWORD sessionid, DWORD parentPid, bool fParent);
