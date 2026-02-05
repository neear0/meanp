#pragma once
#include <Windows.h>

namespace meanp::memory
{
	class c_protection_guard
{
public:
    explicit c_protection_guard(void* address, const std::size_t size) noexcept
        : m_address_{ address }
        , m_size_{ static_cast<DWORD>(size) }
    {
        m_ok_ = static_cast<bool>(
            VirtualProtect(m_address_, m_size_, PAGE_EXECUTE_READWRITE, &m_old_protect_));
    }

    ~c_protection_guard() noexcept
    {
        if (m_ok_)
        {
            DWORD dummy{};
            VirtualProtect(m_address_, m_size_, m_old_protect_, &dummy);
        }
    }

    c_protection_guard(const c_protection_guard&) = delete;
    c_protection_guard& operator=(const c_protection_guard&) = delete;

    bool ok() const noexcept { return m_ok_; }

private:
    void* m_address_{};
    DWORD m_size_{};
    DWORD m_old_protect_{};
    bool  m_ok_{ false };
};
}
