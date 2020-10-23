﻿#include "YaraRuleRunner.h"

#include <fstream>
#include <strsafe.h>
#include <Windows.h>

YaraRuleRunner::YaraRuleRunner(const std::filesystem::path& yara_rules_dir, const std::filesystem::path& raccine_program_directory) :
    m_raccine_program_directory(raccine_program_directory),
    m_yara_rules(get_yara_rules(yara_rules_dir))
{
}

bool YaraRuleRunner::run_yara_rules_on_file(const std::filesystem::path& target_file,
                                            const std::wstring& command_line,
                                            std::wstring& out_yara_output)
{
    for (const std::filesystem::path& yara_rule : m_yara_rules) {
        if (run_yara_rule_on_file(yara_rule, target_file, command_line, out_yara_output)) {
            return true;
        }
    }

    return false;
}

bool YaraRuleRunner::run_yara_rule_on_file(const std::filesystem::path& yara_rule,
                                           const std::filesystem::path& target_file,
                                           const std::wstring& command_line,
                                           std::wstring& out_yara_output) const
{
    std::wstring yara_command_line = m_raccine_program_directory.wstring() + L"\\"
        + YARA_INSTANCE + L" " + yara_rule.wstring() + L" " + target_file.wstring();

    const bool yara_succeeded = run_yara_process(yara_command_line);
    if(!yara_succeeded) {
        return false;
    }

    const std::filesystem::path result_file_path = target_file.wstring() + YARA_RESULTS_SUFFIX;

    // Did we get a match?  allow for an empty newline or two . 
    if (!std::filesystem::exists(result_file_path) || std::filesystem::file_size(result_file_path) < 2) {
        return false;
    }

    const std::wstring yara_output = read_output_file(result_file_path);

    std::filesystem::remove(result_file_path);

    out_yara_output = L"Rule file: " + yara_rule.wstring() + L"\n" + yara_output + L"\n" + L"Command line:\n" + command_line + L"\n\n";

    return true;
}

bool YaraRuleRunner::run_yara_process(std::wstring& command_line)
{
    STARTUPINFO info = { sizeof(info) };
    PROCESS_INFORMATION processInfo{};

    if (!CreateProcessW(
        NULL,
        command_line.data(),
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &info,
        &processInfo)) {
        return false;
    }

    CloseHandle(processInfo.hThread);

    const DWORD wait_result = WaitForSingleObject(processInfo.hProcess, TIMEOUT);
    CloseHandle(processInfo.hProcess);

    if (wait_result == WAIT_TIMEOUT) {
        return false;
    }

    return true;
}

std::wstring YaraRuleRunner::read_output_file(const std::filesystem::path& target_file)
{
    std::ifstream file_stream(target_file);
    const std::string str((std::istreambuf_iterator<char>(file_stream)),
                          std::istreambuf_iterator<char>());
    return std::wstring(str.cbegin(), str.cend());
}

std::vector<std::filesystem::path> YaraRuleRunner::get_yara_rules(const std::filesystem::path& yara_rules_dir)
{
    std::vector<std::filesystem::path> yara_rules;
    const std::wstring ext(L".yar");
    for (const auto& p : std::filesystem::directory_iterator(yara_rules_dir)) {
        if (p.path().extension() == ext) {
            yara_rules.push_back(p.path());
        }
    }
    return yara_rules;
}
