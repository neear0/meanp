#include "utils.hpp"
#include <Windows.h>
#include <string>

std::string meanp::utils::narrow(const std::wstring& input)
{
	auto const size = WideCharToMultiByte(CP_UTF8, 0,
	                                      input.data(), static_cast<std::int32_t>(input.size()),
	                                      nullptr, 0, nullptr, nullptr);

	if (!size)
		return {};

	std::string result(size, '\0');

	WideCharToMultiByte(CP_UTF8, 0,
	                    input.data(), static_cast<std::int32_t>(input.size()),
	                    result.data(), static_cast<std::int32_t>(result.size()),
	                    nullptr, nullptr);

	return result;
}

std::vector<std::string> meanp::utils::get_argv()
{
	std::int32_t argc{};
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (!argv)
		return {};

	std::vector<std::string> result;
	result.reserve(argc);

	for (std::int32_t i = 0; i < argc; ++i)
		result.emplace_back(narrow(argv[i]));

	LocalFree(argv);
	return result;
}

std::unordered_map<std::string, std::uint8_t*> meanp::utils::resolve_dll_imports(
	const c_console& log,
	std::string_view module,
	const std::vector<std::string>& names
)
{
	const HMODULE handle = GetModuleHandleA(module.data());

	if (!handle)
	{
		log.log_error(std::format("Failed to get module handle for '{}'", module));
		return {};
	}

	std::unordered_map<std::string, std::uint8_t*> result;
	result.reserve(names.size());

	for (const auto& name : names)
	{
		auto* addr = reinterpret_cast<std::uint8_t*>(GetProcAddress(handle, name.c_str()));

		if (!addr)
		{
			log.log_error(std::format("Failed to resolve '{}' in '{}'", name, module));
			return {};
		}

		result.emplace(name, addr);
	}

	return result;
}
