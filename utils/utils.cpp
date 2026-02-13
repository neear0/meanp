#include "utils.hpp"
#include "../core/parser.hpp"
#include <filesystem>
#include <fstream>
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

std::vector<std::filesystem::path> meanp::utils::collect_files_from_config(
	const std::filesystem::path& config_path,
	const c_console& log)
{
	auto result = std::vector<std::filesystem::path>{};

	if (!std::filesystem::exists(config_path))
		return result;

	std::ifstream file{ config_path };
	if (!file)
	{
		log.log_warn(std::format("Could not open config file '{}'", config_path.string()));
		return result;
	}

	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty() || line.starts_with('#') || line.starts_with(';'))
			continue;

		const auto start = line.find_first_not_of(" \t");
		const auto end = line.find_last_not_of(" \t\r\n");
		if (start == std::string::npos)
			continue;

		auto path = std::filesystem::path{ line.substr(start, end - start + 1) };

		if (!std::filesystem::exists(path))
		{
			log.log_warn(std::format("Patch file '{}' does not exist", path.string()));
			continue;
		}

		result.push_back(std::move(path));
	}

	return result;
}

std::vector<std::filesystem::path> meanp::utils::collect_files_from_directory(const std::filesystem::path& dir)
{
	auto result = std::vector<std::filesystem::path>{};

	if (!is_directory(dir))
		return result;

	for (auto&& entry : std::filesystem::directory_iterator(dir))
		if (entry.is_regular_file() && entry.path().extension() == ".mph")
			result.push_back(entry.path());

	return result;
}

auto meanp::utils::load_all(
	const std::vector<std::filesystem::path>& files,
	const c_console& log) -> std::optional<std::vector<patch_t>>
{
	auto patches = std::vector<patch_t>{};

	for (auto&& file : files)
	{
		log.log_info(std::format("Opening memory patch file '{}'", file.string()));

		auto result = read_file(file);
		if (!result)
		{
			log.log_error(std::format(
				"Parsing failed on line {}: {}",
				result.error().line, make_error_code(result.error().ec).message()));
			return std::nullopt;
		}

		for (auto&& patch : *result)
		{
			log_patch_info(patch, log);
			patches.push_back(std::move(patch));
		}
	}

	return patches;
}

void meanp::utils::log_patch_info(const patch_t& patch, const c_console& log)
{
	log.log_info(std::format(
		"Parsed {} from '{}':{} at {} {}",
		patch.on.empty() ? "check" : "patch",
		patch.file, patch.line,
		patch.type_name(), patch.target_name()));

	if (!patch.off.empty())
	{
		log.log_info(std::format(
			"     expected data [{}]: {}",
			patch.off.size(), format_hex(patch.off)));
	}

	if (!patch.on.empty())
	{
		log.log_info(std::format(
			"  replacement data [{}]: {}",
			patch.on.size(), format_hex(patch.on)));
	}
}

std::string meanp::utils::format_hex(const std::vector<std::uint8_t>& data)
{
	std::string result;
	result.reserve(data.size() * 3);

	for (std::size_t i = 0; i < data.size(); ++i)
	{
		if (i > 0) result += ' ';
		result += std::format("{:02X}", data[i]);
	}

	return result;
}

std::string meanp::utils::get_host_exe()
{
	auto name = std::wstring(MAX_PATH, L'\0');
	auto const size = GetModuleFileNameW(nullptr, name.data(), name.size());

	if (size == 0)
		return "<unknown>";

	name.resize(size);
	return std::filesystem::path{ name }.filename().string();
}

std::filesystem::path meanp::utils::get_dll_directory()
{
	auto path = std::wstring(MAX_PATH, L'\0');
	auto const size = GetModuleFileNameW(
		GetModuleHandleW(L"meanp.dll"),
		path.data(),
		path.size());

	if (size == 0)
		return std::filesystem::current_path();

	path.resize(size);
	return std::filesystem::path{ path }.parent_path();
}

std::unordered_map<std::string, std::uint8_t*> meanp::utils::resolve_dll_imports(
	const c_console& log,
	std::string_view module,
	const std::vector<std::string>& names)
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

