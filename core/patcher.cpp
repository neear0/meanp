#include "patcher.hpp"

#include <span>
#include <utility>

#include "c_protection_guard.hpp"

meanp::memory::c_patcher::c_patcher(const utils::c_console& log) noexcept
    : m_log_{ log }
{
}

bool meanp::memory::c_patcher::apply(std::uint8_t* base, const parser::patch_t& patch) const
{
    auto* address = resolve_address(base, patch);
    if (!address)
        return false;

    if (!patch.off.empty() && !compare_data(address, patch))
        return false;

    if (patch.on.empty())
    {
        m_log_.log_info(std::format(
            "Validated data from '{}':{}  at {}  {} (0x{:X})",
            patch.file, patch.line, patch.type_name(), patch.target_name(),
            reinterpret_cast<std::uintptr_t>(address)));
        return true;
    }

    return write_data(address, patch);
}


std::uint8_t* meanp::memory::c_patcher::file_offset_to_ptr(std::uint8_t* base, const std::uintptr_t offset)
{
    const auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return nullptr;

    auto* nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return nullptr;

    const auto* sections = IMAGE_FIRST_SECTION(nt);

    if (offset < sections->PointerToRawData)
        return base + offset;

    for (int i = 0; std::cmp_less(i, nt->FileHeader.NumberOfSections); ++i)
    {
	    if (const auto* sec = sections + i; offset >= sec->PointerToRawData &&
            offset < sec->PointerToRawData + sec->SizeOfRawData)
        {
            return base + sec->VirtualAddress + (offset - sec->PointerToRawData);
        }
    }

    return nullptr;
}

std::uint8_t* meanp::memory::c_patcher::resolve_address(std::uint8_t* base, const parser::patch_t& patch) const
{
    switch (patch.type)
    {
    case parser::addr_type::absolute:
        return reinterpret_cast<std::uint8_t*>(patch.address);

    case parser::addr_type::rva:
        return base + patch.address;

    case parser::addr_type::file:
    {
        auto* result = file_offset_to_ptr(base, patch.address);
        if (!result)
        {
            m_log_.log_error(std::format(
                "Failed to convert file offset 0x{:X} to RVA  ('{}':{})",
                patch.address, patch.file, patch.line));
        }
        return result;
    }
    }

    m_log_.log_error(std::format(
        "Unknown address type ({})  ('{}':{})",
        static_cast<std::uint8_t>(patch.type), patch.file, patch.line));
    return nullptr;
}

bool meanp::memory::c_patcher::compare_data(std::uint8_t* target, const parser::patch_t& patch) const
{
    const std::span expected{ patch.off };
    const std::span actual{ target, expected.size() };

    if (std::ranges::equal(expected, actual))
        return true;

    if (!patch.on.empty() &&
        std::memcmp(target, patch.on.data(), patch.on.size()) == 0)
    {
        m_log_.log_warn(std::format(
            "Patch from '{}':{}  at {}  0x{:X} has already been applied",
            patch.file, patch.line, patch.type_name(), patch.address));
        return true;
    }

    auto hex_dump = [](const std::span<const std::uint8_t> data) -> std::string
        {
            std::string out;
            out.reserve(data.size() * 3);
            for (std::size_t i = 0; i < data.size(); ++i)
            {
                if (i) out += ' ';
                out += std::format("{:02X}", data[i]);
            }
            return out;
        };

    m_log_.log_error(std::format(
        "Data validation failed  ('{}':{}  at {}  0x{:X})\n"
        "     expected [{}]: {}\n"
        "     actual   [{}]: {}",
        patch.file, patch.line, patch.type_name(), patch.address,
        expected.size(), hex_dump(expected),
        actual.size(), hex_dump(actual)));

    return false;
}

bool meanp::memory::c_patcher::write_data(std::uint8_t* target, const parser::patch_t& patch) const
{
	if (const c_protection_guard guard{ target, patch.on.size() }; !guard.ok())
    {
        m_log_.log_error(std::format(
            "VirtualProtect failed for patch '{}':{}  at {}  0x{:X}",
            patch.file, patch.line, patch.type_name(), patch.address));
        return false;
    }

    std::memcpy(target, patch.on.data(), patch.on.size());

    std::string msg = std::format(
        "Applied patch from '{}':{}  at {}  {}",
        patch.file, patch.line, patch.type_name(), patch.target_name());

    if (patch.type != parser::addr_type::absolute)
        msg += std::format(" -> 0x{:X}", reinterpret_cast<std::uintptr_t>(target));

    m_log_.log_info(msg);
    return true;
}
