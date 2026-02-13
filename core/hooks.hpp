#pragma once
#include <vector>
#include <Windows.h>
#include <functional>

#include "patcher.hpp"
#include "nt_layer.hpp"

namespace meanp
{
    class c_dll_hook_manager final
    {
    public:
        using patch_list = std::vector<patch_t>;
        using unload_callback = std::function<void()>;

        explicit c_dll_hook_manager(
            HMODULE module,
            std::vector<patch_t> patches,
            std::function<void()> on_unload = nullptr
        ) noexcept;

        ~c_dll_hook_manager() noexcept;

        c_dll_hook_manager(const c_dll_hook_manager&) = delete;
        c_dll_hook_manager& operator=(const c_dll_hook_manager&) = delete;
        c_dll_hook_manager(c_dll_hook_manager&&) noexcept = default;
        c_dll_hook_manager& operator=(c_dll_hook_manager&&) noexcept = default;

        bool place() const;

        bool all_patches_applied() const noexcept;
        std::size_t pending_patches() const noexcept;

    private:
        struct impl;
        std::unique_ptr<impl> pimpl_;

        static void __stdcall dll_notification_callback(
            ULONG reason,
            type_dll_notification_data data,
            PVOID context
        );

        static std::uint8_t* resolve_module_base(const std::string& target);

        static std::optional<bool> try_apply_loaded_module(
            const c_patcher& patcher,
            const patch_t& patch
        );
    };
}