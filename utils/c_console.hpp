#pragma once
#include <string_view>
#include <Windows.h>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <vector>
#include <tlhelp32.h>
#include <algorithm>
#include <mutex>

namespace meanp
{
    enum class console_color : std::uint8_t
    {
        black = 0,
        blue = 1,
        green = 2,
        aqua = 3,
        red = 4,
        purple = 5,
        yellow = 6,
        white = 7,
        gray = 8,
        light_blue = 9,
        light_green = 10,
        light_aqua = 11,
        light_red = 12,
        light_purple = 13,
        light_yellow = 14,
        bright_white = 15
    };

    class c_console
    {
    public:
        explicit c_console(std::string_view window_name = "");
        ~c_console();

        c_console(const c_console&) = delete;
        c_console& operator=(const c_console&) = delete;

        c_console(c_console&&) noexcept = default;
        c_console& operator=(c_console&&) noexcept = default;

        void write(console_color text_color, std::string_view message) const;
        void write(std::string_view message) const;
        void write_timestamped(console_color text_color, std::string_view message) const;
        void clear() const;
        void set_title(std::string_view title);

		bool is_attached() const noexcept { return m_attached_; }
        static std::string get_local_time(std::string_view format = "[%H:%M:%S] ");

    private:
        bool attach();
        void detach();

        HANDLE m_console_handle_{ INVALID_HANDLE_VALUE };
        bool m_attached_{ false };
        std::string m_window_name_;
        mutable std::mutex m_mutex_; 

        static constexpr auto default_color = console_color::white;
    };
}
