#pragma once
#include <Windows.h>
#include <cstdint>
#include <string_view>
#include <vector>

#include "parser.hpp"
#include "../utils/console.hpp"

namespace meanp
{


	class c_patcher
    {
    public:
        explicit c_patcher(const utils::c_console& log) noexcept;
        bool apply(std::uint8_t* base, const patch_t& patch) const;
    private:
        static std::uint8_t* file_offset_to_ptr(std::uint8_t* base, std::uintptr_t offset);
        std::uint8_t* resolve_address(std::uint8_t* base, const patch_t& patch) const;
        bool compare_data(std::uint8_t* target, const patch_t& patch) const;
        bool write_data(std::uint8_t* target, const patch_t& patch) const;
        const utils::c_console& m_log_;
    };
}
