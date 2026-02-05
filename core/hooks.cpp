#include "hooks.hpp"
#include "../utils/utils.hpp"
#include <ranges>

struct meanp::c_dll_hook_manager::impl
{
    HMODULE module_handle{ nullptr };
    patch_list pending_patches{};
    unload_callback unload_fn{};

    PVOID notification_cookie{ nullptr };
    decltype(&::register_dll_notification) register_fn{ nullptr };
    decltype(&::unregister_dll_notification) unregister_fn{ nullptr };

    // FIX: Store c_console by value instead of passing temporary to c_patcher
    utils::c_console console;
    memory::c_patcher patcher;
    std::mutex patches_mutex{};

    explicit impl(HMODULE module, patch_list patches, unload_callback callback)
        : module_handle(module)
        , pending_patches(std::move(patches))
        , unload_fn(std::move(callback))
        , console()  // Default construct console
        , patcher(console)  // Pass by reference to c_patcher
    {
    }

    ~impl() noexcept
    {
        unregister_notification();
    }

    bool register_notification()
    {
        if (!resolve_ntdll_functions())
            return false;

        if (const auto status = register_fn(0, dll_notification_callback, this, &notification_cookie); !NT_SUCCESS(status))
        {
            // FIX: Access console through stored member
            console.log_error(std::format(
                "Failed to register DLL notification callback (0x{:X})", status));
            return false;
        }

        return true;
    }

    void unregister_notification() noexcept
    {
        if (!notification_cookie || !unregister_fn)
            return;

        if (const auto status = unregister_fn(notification_cookie); !NT_SUCCESS(status))
        {
            console.log_error(std::format(
                "Failed to unregister DLL notification (0x{:X})", status));
        }

        notification_cookie = nullptr;
    }

    bool resolve_ntdll_functions()
    {
        const auto imports = utils::resolve_dll_imports(
            console, "ntdll.dll",
            { "LdrRegisterDllNotification", "LdrUnregisterDllNotification" }
        );

        if (imports.size() != 2)
        {
            console.log_error("Failed to resolve ntdll.dll imports");
            return false;
        }

        register_fn = reinterpret_cast<decltype(register_fn)>(
            imports.at("LdrRegisterDllNotification")
            );
        unregister_fn = reinterpret_cast<decltype(unregister_fn)>(
            imports.at("LdrUnregisterDllNotification")
            );

        return true;
    }

    void on_dll_loaded(std::string_view module_name, std::uint8_t* base_address)
    {
        std::scoped_lock lock(patches_mutex);

        const bool has_patches = std::ranges::any_of(
            pending_patches,
            [module_name](const auto& patch) { return patch.target == module_name; }
        );

        if (!has_patches)
            return;

        console.log_info(std::format(
            "Loaded target file '{}' at address 0x{:X}",
            module_name, reinterpret_cast<std::uintptr_t>(base_address)));

        apply_patches_for_module(module_name, base_address);
    }

    void apply_patches_for_module(const std::string_view module_name, std::uint8_t* base_address)
    {
        std::vector<std::size_t> applied_indices;

        for (std::size_t i = 0; i < pending_patches.size(); ++i)
        {
            auto& patch = pending_patches[i];

            if (patch.target != module_name)
                continue;

            if (!patcher.apply(base_address, patch))
                break;

            applied_indices.push_back(i);
        }

        for (const auto idx : applied_indices | std::views::reverse)
            pending_patches.erase(pending_patches.begin() + static_cast<std::ptrdiff_t>(idx));

        check_completion();
    }

    void check_completion() const
    {
        if (!pending_patches.empty())
            return;

        console.log_info("All patches applied, unloading from process...");

        if (unload_fn)
        {
            CreateThread(
                nullptr, 0,
                [](LPVOID param) -> DWORD {
                    auto* callback = static_cast<unload_callback*>(param);
                    (*callback)();
                    delete callback;
                    return 0;
                },
                new unload_callback(unload_fn),
                0, nullptr
            );
        }
    }
};

auto __stdcall meanp::c_dll_hook_manager::dll_notification_callback(
    const ULONG reason,
    const type_dll_notification_data data,
    const PVOID context
) -> void
{
    if (reason != dll_notification_reason_loaded || !context)
        return;

    auto* impl_ptr = static_cast<impl*>(context);

    const auto base_address = static_cast<std::uint8_t*>(data->loaded.dll_base);
    const auto module_name = utils::narrow({
        data->loaded.base_dll_name->Buffer,
        data->loaded.base_dll_name->Length / sizeof(wchar_t)
        });

    impl_ptr->on_dll_loaded(module_name, base_address);
}

std::uint8_t* meanp::c_dll_hook_manager::resolve_module_base(const std::string& target)
{
    if (target == "-")
        return nullptr;

    if (target == "<host>")
    {
        const auto handle = ::GetModuleHandleW(nullptr);
        return reinterpret_cast<std::uint8_t*>(handle);
    }

    const auto handle = ::GetModuleHandleA(target.c_str());
    return handle ? reinterpret_cast<std::uint8_t*>(handle) : nullptr;
}

std::optional<bool> meanp::c_dll_hook_manager::try_apply_loaded_module(
    const memory::c_patcher& patcher,
    parser::patch_t& patch
)
{
    auto* base = resolve_module_base(patch.target);

    if (patch.target == "-")
        return patcher.apply(nullptr, patch);

    if (!base)
        return std::nullopt;

    // NOTE: This will cause a compilation error because m_log_ is private
    // You need to either:
    // 1. Add a public get_log() method to c_patcher, or
    // 2. Pass the console instance to this function
    // For now, I'll comment this out and show you the alternative

    /*
    static std::vector<std::string> logged_modules;
    if (std::ranges::find(logged_modules, patch.target) == logged_modules.end())
    {
        patcher.m_log_.log_info(std::format(
            "Target module '{}' resolved to address 0x{:X}",
            patch.target, reinterpret_cast<std::uintptr_t>(base)));
        logged_modules.push_back(patch.target);
    }
    */

    return patcher.apply(base, patch);
}

meanp::c_dll_hook_manager::c_dll_hook_manager(
    HMODULE module,
    patch_list patches,
    unload_callback on_unload
) noexcept
    : pimpl_(std::make_unique<impl>(module, std::move(patches), std::move(on_unload)))
{
}

meanp::c_dll_hook_manager::~c_dll_hook_manager() noexcept = default;

bool meanp::c_dll_hook_manager::place() const
{
    if (!pimpl_)
        return false;

    std::scoped_lock lock(pimpl_->patches_mutex);

    std::vector<std::size_t> applied_indices;

    for (std::size_t i = 0; i < pimpl_->pending_patches.size(); ++i)
    {
        auto& patch = pimpl_->pending_patches[i];
        auto result = try_apply_loaded_module(pimpl_->patcher, patch);

        if (!result.has_value())
            continue;

        if (!*result)
            return false;

        applied_indices.push_back(i);
    }

    for (const auto idx : applied_indices | std::views::reverse)
        pimpl_->pending_patches.erase(pimpl_->pending_patches.begin() + static_cast<std::ptrdiff_t>(idx));

    if (pimpl_->pending_patches.empty())
    {
        pimpl_->check_completion();
        return true;
    }

    return pimpl_->register_notification();
}

bool meanp::c_dll_hook_manager::all_patches_applied() const noexcept
{
    return pimpl_ && pimpl_->pending_patches.empty();
}

std::size_t meanp::c_dll_hook_manager::pending_patches() const noexcept
{
    return pimpl_ ? pimpl_->pending_patches.size() : 0;
}


