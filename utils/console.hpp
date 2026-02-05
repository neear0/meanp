#pragma once
#include <string_view>
#include <Windows.h>
#include <mutex>

namespace meanp::utils
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
        bright_white = 15,
    };

    enum class log_level : std::uint8_t
    {
        info = 0,
        warn = 1,
        error = 2,
    };

    class c_console
    {
    public:
        explicit c_console(std::string_view window_name = {});
        ~c_console();

        c_console(const c_console&) = delete;
        c_console& operator=(const c_console&) = delete;
        c_console(c_console&&) = delete;
        c_console& operator=(c_console&&) = delete;

        bool is_attached() const noexcept;

        void write(console_color color, std::string_view message) const;
        void write(std::string_view message) const;
        void write_timestamped(console_color color, std::string_view message) const;

        void log_info(std::string_view message) const;
        void log_warn(std::string_view message) const;
        void log_error(std::string_view message) const;

        void clear() const;
        void set_title(std::string_view title);

        static std::string local_time(std::string_view fmt = "[%H:%M:%S] ");

    private:
        bool attach();
        void detach();

        HANDLE      m_handle_{ INVALID_HANDLE_VALUE };
        bool        m_attached_{ false };
        std::string m_title_;
        mutable std::mutex m_mutex_;

        static constexpr console_color default_color = console_color::white;
    };

}
