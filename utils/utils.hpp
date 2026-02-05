#pragma once
#include <unordered_map>
#include "console.hpp"

namespace meanp::utils
{
    std::string narrow(const std::wstring& input);
    std::vector<std::string> get_argv();

    std::unordered_map<std::string, std::uint8_t*> resolve_dll_imports( const c_console& log, std::string_view module, const std::vector<std::string>& names);

}