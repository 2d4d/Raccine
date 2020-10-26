﻿#pragma once
#include <filesystem>
#include <vector>
#include <Windows.h>

#define YARA_RESULTS_SUFFIX L".out"
#define YARA_INSTANCE  L"runyara.bat"
#define YARA_USERCONTEXT_DIR  L"%TEMP%\\RaccineUserContext"
inline WCHAR g_wRaccineUserContextDirectory[MAX_PATH] = { 0 };  // ENV expanded RACCINE_DATA_DIRECTORY

constexpr UINT TIMEOUT = 5000;

class YaraRuleRunner final
{
public:
    [[nodiscard]] YaraRuleRunner(const std::filesystem::path& yara_rules_dir,
                                 const std::filesystem::path& raccine_program_directory);

    ~YaraRuleRunner() = default;

    [[nodiscard]] bool run_yara_rules_on_file(const std::filesystem::path& target_file,
                                              const std::wstring& command_line,
                                              std::wstring& out_yara_output,
                                              std::wstring& yara_cmd_optional_defines);

    // Deleted functions
    YaraRuleRunner(const YaraRuleRunner&) = delete;
    YaraRuleRunner& operator=(const YaraRuleRunner&) = delete;
    YaraRuleRunner(YaraRuleRunner&&) = delete;
    YaraRuleRunner& operator=(YaraRuleRunner&&) = delete;

private:

    bool run_yara_rule_on_file(const std::filesystem::path& yara_rule,
                               const std::filesystem::path& target_file,
                               const std::wstring& command_line,
                               std::wstring& out_yara_output,
                               std::wstring& yara_cmd_optional_defines) const;

    static bool run_yara_process(std::wstring& command_line);

    static std::wstring read_output_file(const std::filesystem::path& target_file);

    static std::vector<std::filesystem::path> get_yara_rules(const std::filesystem::path& yara_rules_dir);

    const std::filesystem::path m_raccine_program_directory;
    std::vector<std::filesystem::path> m_yara_rules;
};
