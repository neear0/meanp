#pragma once
#include <filesystem>
#include <unordered_map>
#include "console.hpp"

namespace meanp
{
	struct patch_t;
}

namespace meanp::utils
{
    std::string narrow(const std::wstring& input);
    std::vector<std::string> get_argv();
    std::vector<std::filesystem::path> collect_files_from_config(const std::filesystem::path& config_path,const c_console& log);
    std::vector<std::filesystem::path> collect_files_from_directory(const std::filesystem::path& dir);
    std::optional<std::vector<patch_t>> load_all(const std::vector<std::filesystem::path>& files, const c_console& log);
    void log_patch_info(const patch_t& patch, const c_console& log);
    std::string format_hex(const std::vector<std::uint8_t>& data);
    std::string get_host_exe();
    std::filesystem::path get_dll_directory();
    std::unordered_map<std::string, std::uint8_t*> resolve_dll_imports(const c_console& log, std::string_view module, const std::vector<std::string>& names);
}
