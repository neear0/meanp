#include <Windows.h>

#include "core/parser.hpp"
#include "utils/utils.hpp"
#include "core/patcher.hpp"
#include "core/hooks.hpp"
#include "utils/console.hpp"

int __stdcall DllMain(HINSTANCE module, const DWORD reason, LPVOID)
{
    if (reason != DLL_PROCESS_ATTACH)
        return TRUE;

    DisableThreadLibraryCalls(module);

    const auto console = meanp::utils::c_console{"meanp"};

    console.log_info(std::format(
        "meanp loaded in '{}' at 0x{:X}",
        meanp::utils::get_host_exe(),
        std::bit_cast<std::uintptr_t>(module)));

    auto const dll_dir = meanp::utils::get_dll_directory();
    auto files = std::vector<std::filesystem::path>{};

    if (const std::filesystem::path config_file = dll_dir / "patches.txt"; std::filesystem::exists(config_file))
    {
        console.log_info(std::format("Loading patches from config: {}", config_file.string()));
        files = meanp::utils::collect_files_from_config(config_file, console);
    }

    const auto autopatch_dir = dll_dir / "autopatch";
    std::ranges::move(
        meanp::utils::collect_files_from_directory(autopatch_dir),
        std::back_inserter(files));

    if (files.empty())
    {
        console.log_warn("No patch files could be found");
        console.log_warn(std::format("Create a 'patches.txt' file in '{}' with paths to .mph files", dll_dir.string()));
        console.log_warn(std::format("Or place .mph files in '{}'", autopatch_dir.string()));
        console.log_warn(std::format("Exiting.....", autopatch_dir.string()));
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return FALSE;
    }

    const auto patches = meanp::utils::load_all(files, console);
    if (!patches)
        return FALSE;

    const auto manager = meanp::c_dll_hook_manager(module,*patches,[module] { FreeLibraryAndExitThread(module, EXIT_SUCCESS); }
    );

    if (!manager.place())
        return FALSE;

    return TRUE;
}