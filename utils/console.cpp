#pragma once
#include <iostream>

#include "c_console.hpp"

meanp::c_console::c_console(const std::string_view window_name)
    : m_window_name_(window_name)
{
    m_attached_ = attach();
}

meanp::c_console::~c_console()
{
    if (m_attached_)
        detach();
}

bool meanp::c_console::attach()
{
    if (m_attached_)
        return true;

    if (!AllocConsole())
    {
	    if (const auto error = GetLastError(); error != ERROR_ACCESS_DENIED)
            return false;
    }

    FILE* fp_stdin = nullptr;
    FILE* fp_stdout = nullptr;
    FILE* fp_stderr = nullptr;

    if (freopen_s(&fp_stdin, "CONIN$", "r", stdin) != 0 ||
        freopen_s(&fp_stdout, "CONOUT$", "w", stdout) != 0 ||
        freopen_s(&fp_stderr, "CONOUT$", "w", stderr) != 0)
    {
        FreeConsole();
        return false;
    }

    std::ios::sync_with_stdio(true);

    m_console_handle_ = GetStdHandle(STD_OUTPUT_HANDLE);
    if (m_console_handle_ == INVALID_HANDLE_VALUE)
    {
        FreeConsole();
        return false;
    }

    SetConsoleTextAttribute(m_console_handle_, static_cast<WORD>(default_color));

    if (!m_window_name_.empty())
        SetConsoleTitleA(m_window_name_.c_str());

    clear();
    return true;
}

inline void meanp::c_console::detach()
{
    if (!m_attached_)
        return;

    if (stdin)  fclose(stdin);
    if (stdout) fclose(stdout);
    if (stderr) fclose(stderr);

    FreeConsole();
    m_console_handle_ = INVALID_HANDLE_VALUE;
    m_attached_ = false;
}

void meanp::c_console::write(console_color text_color, std::string_view message) const
{
    if (!m_attached_ || m_console_handle_ == INVALID_HANDLE_VALUE)
        return;

    std::scoped_lock lock(m_mutex_);

    SetConsoleTextAttribute(m_console_handle_, static_cast<WORD>(text_color));

    std::cout << message << '\n';

    SetConsoleTextAttribute(m_console_handle_, static_cast<WORD>(default_color));
}

void meanp::c_console::write(const std::string_view message) const
{
    write(default_color, message);
}

void meanp::c_console::write_timestamped(const console_color text_color, std::string_view message) const
{
    const auto timestamp = get_local_time();
    const auto full_message = std::format("{}{}", timestamp, message);
    write(text_color, full_message);
}

inline void meanp::c_console::clear() const
{
    if (!m_attached_)
        return;

    std::scoped_lock lock(m_mutex_);

    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    DWORD cells_written;
    constexpr COORD home_coords = { 0, 0 };

    if (!GetConsoleScreenBufferInfo(m_console_handle_, &buffer_info))
        return;

    const DWORD con_size = buffer_info.dwSize.X * buffer_info.dwSize.Y;

    FillConsoleOutputCharacter(m_console_handle_, ' ', con_size, home_coords, &cells_written);
    FillConsoleOutputAttribute(m_console_handle_, buffer_info.wAttributes, con_size, home_coords, &cells_written);
    SetConsoleCursorPosition(m_console_handle_, home_coords);
}

inline void meanp::c_console::set_title(const std::string_view title)
{
    if (!m_attached_)
        return;

    std::scoped_lock lock(m_mutex_);
    m_window_name_ = title;
    SetConsoleTitleA(m_window_name_.c_str());
}

inline std::string meanp::c_console::get_local_time(const std::string_view format)
{
    const auto now = std::chrono::system_clock::now();
    const auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::tm local_time;
    localtime_s(&local_time, &time_t_now);

    std::ostringstream oss;
    oss << std::put_time(&local_time, format.data());
    return oss.str();
}