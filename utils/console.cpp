#include <iostream>
#include "console.hpp"
#include <iomanip>
#include <sstream>

meanp::utils::c_console::c_console(const std::string_view window_name)
    : m_title_{ window_name }
{
    m_attached_ = attach();
}

meanp::utils::c_console::~c_console()
{
    detach();          
}

bool meanp::utils::c_console::is_attached() const noexcept
{
    return m_attached_;
}


void meanp::utils::c_console::write(console_color color, const std::string_view message) const
{
    if (!m_attached_)
        return;

    std::scoped_lock lock{ m_mutex_ };

    SetConsoleTextAttribute(m_handle_, static_cast<WORD>(color));

    const DWORD len = message.size();
    DWORD       written{};
    WriteConsoleA(m_handle_, message.data(), len, &written, nullptr);

    // newline
    WriteConsoleA(m_handle_, "\n", 1u, &written, nullptr);

    SetConsoleTextAttribute(m_handle_, static_cast<WORD>(default_color));
}

void meanp::utils::c_console::write(const std::string_view message) const
{
    write(default_color, message);
}

void meanp::utils::c_console::write_timestamped(const console_color color, std::string_view message) const
{
    write(color, std::format("{}{}", local_time(), message));
}

void meanp::utils::c_console::log_info(std::string_view message) const
{
    write_timestamped(console_color::white, std::format("[INFO]  {}", message));
}

void meanp::utils::c_console::log_warn(std::string_view message) const
{
    write_timestamped(console_color::yellow, std::format("[WARN]  {}", message));
}

void meanp::utils::c_console::log_error(std::string_view message) const
{
    write_timestamped(console_color::light_red, std::format("[ERROR] {}", message));
}

void meanp::utils::c_console::clear() const
{
    if (!m_attached_)
        return;

    std::scoped_lock lock{ m_mutex_ };

    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(m_handle_, &info))
        return;

    constexpr COORD  origin{ 0, 0 };
    const     DWORD  cells = static_cast<DWORD>(info.dwSize.X) *
        static_cast<DWORD>(info.dwSize.Y);
    DWORD written{};

    FillConsoleOutputCharacter(m_handle_, L' ', cells, origin, &written);
    FillConsoleOutputAttribute(m_handle_, info.wAttributes, cells, origin, &written);
    SetConsoleCursorPosition(m_handle_, origin);
}

void meanp::utils::c_console::set_title(const std::string_view title)
{
    if (!m_attached_)
        return;

    std::scoped_lock lock{ m_mutex_ };
    m_title_ = title;
    SetConsoleTitleA(m_title_.c_str());
}

std::string meanp::utils::c_console::local_time(const std::string_view fmt)
{
    const auto now = std::chrono::system_clock::now();
    const auto time_t = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
    localtime_s(&tm, &time_t);

    std::ostringstream oss;
    oss << std::put_time(&tm, fmt.data());
    return oss.str();
}

bool meanp::utils::c_console::attach()
{
    if (m_attached_)
        return true;

    if (!AllocConsole() && GetLastError() != ERROR_ACCESS_DENIED)
        return false;

    FILE* dummy{};
    if (freopen_s(&dummy, "CONIN$", "r", stdin) != 0 ||
        freopen_s(&dummy, "CONOUT$", "w", stdout) != 0 ||
        freopen_s(&dummy, "CONOUT$", "w", stderr) != 0)
    {
        FreeConsole();
        return false;
    }

    m_handle_ = GetStdHandle(STD_OUTPUT_HANDLE);
    if (m_handle_ == INVALID_HANDLE_VALUE)
    {
        FreeConsole();
        return false;
    }

    SetConsoleTextAttribute(m_handle_, static_cast<WORD>(default_color));

    if (!m_title_.empty())
        SetConsoleTitleA(m_title_.c_str());


    m_attached_ = true;
    clear();

    return true;
}

void meanp::utils::c_console::detach()
{
    if (!m_attached_)
        return;

    std::fclose(stdin);
    std::fclose(stdout);
    std::fclose(stderr);

    FreeConsole();

    m_handle_ = INVALID_HANDLE_VALUE;
    m_attached_ = false;
}
