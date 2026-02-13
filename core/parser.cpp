#include "parser.hpp"

#include <fstream>
#include <ranges>

std::string_view meanp::patch_t::type_name() const
{
    switch (type)
    {
    case addr_type::absolute: return "absolute address";
    case addr_type::rva:      return "RVA";
    case addr_type::file:     return "file";
    }
    return "???";
}

std::string meanp::patch_t::target_name() const
{
    if (type == addr_type::absolute)
        return std::format("0x{:X}", address);
    return std::format("'{}'+0x{:X}", target, address);
}

std::expected<meanp::read_target_result_t, meanp::parser_errors> meanp::read_target(std::string_view line)
{
    const bool   quoted = line.starts_with('"');
    const size_t start = quoted ? 1u : 0u;
    const size_t end = line.find(quoted ? '"' : ' ', start);

    if (end == std::string_view::npos)
        return std::unexpected{ quoted ? parser_errors::parse_target_unclosed_quote
                                       : parser_errors::parse_insufficient_args };

    return read_target_result_t{
        .name = std::string{ line.substr(start, end - start) },
        .offset = end + 1
    };
}

std::expected<meanp::read_offset_result_t, meanp::parser_errors> meanp::read_offset(const std::string_view token)
{
    const bool is_file = token.size() >= 2 &&
        (token[0] == 'f' || token[0] == 'F') &&
        token[1] == '+';

    read_offset_result_t result{ .type = is_file ? addr_type::file : addr_type::rva };

    const char* start = token.data() + (is_file ? 2 : 0);
    const char* end = token.data() + token.size();

    if (const auto [ptr, ec] = std::from_chars(start, end, result.address, 16); ec != std::errc{} || ptr != end)
        return std::unexpected{ parser_errors::parse_bad_offset_address };

    return result;
}

std::expected<std::vector<std::uint8_t>, meanp::parser_errors> meanp::read_data(const std::string_view hex)
{
    if (hex.size() % 2 != 0)
        return std::unexpected{ parser_errors::parse_bad_data_length };

    std::vector<std::uint8_t> result(hex.size() / 2);

    for (std::size_t i = 0; i < hex.size(); i += 2)
    {
        const auto [ptr, ec] =
            std::from_chars(hex.data() + i, hex.data() + i + 2, result[i / 2], 16);

        if (ec != std::errc{})
            return std::unexpected{ parser_errors::parse_bad_data_bytes };
    }

    return result;
}

std::expected<meanp::patch_t, meanp::parser_errors> meanp::read_line(std::string_view line)
{
    if (line.empty() ||
        line.starts_with('#') ||
        line.find_first_not_of(" \t") == std::string_view::npos)
    {
        return std::unexpected{ parser_errors::parse_line_empty };
    }

    std::expected<read_target_result_t, parser_errors> target = read_target(line);
    if (!target)
        return std::unexpected{ target.error() };

    patch_t result{ .target = std::move(target->name) };

    const std::string_view remainder{ line.substr(target->offset) };

    const std::vector<std::string> args = remainder
        | std::views::split(' ')
        | std::views::transform([](auto&& r) { return std::string_view{ r.begin(), r.end() }; })
        | std::views::filter([](std::string_view v) { return !v.empty(); })
        | std::ranges::to<std::vector<std::string>>();

    if (args.size() < 2)
        return std::unexpected{ parser_errors::parse_insufficient_args };
    if (args.size() > 3)
        return std::unexpected{ parser_errors::parse_too_many_args };

    std::expected<read_offset_result_t, parser_errors> offset = read_offset(args[0]);
    if (!offset)
        return std::unexpected{ offset.error() };

    result.type = offset->type;
    result.address = offset->address;

    if (result.target == "-")
        result.type = addr_type::absolute;

    if (args[1] != "-")
    {
        std::expected<std::vector<std::uint8_t>, parser_errors> on = read_data(args[1]);
        if (!on)
            return std::unexpected{ on.error() };
        result.on = std::move(*on);
    }

    if (args.size() > 2)
    {
        std::expected<std::vector<std::uint8_t>, parser_errors> off = read_data(args[2]);
        if (!off)
            return std::unexpected{ off.error() };
        result.off = std::move(*off);
    }

    if (result.on.empty() && result.off.empty())
        return std::unexpected{ parser_errors::parse_insufficient_args };

    return result;
}


std::expected<std::vector<meanp::patch_t>, meanp::parse_error_t> meanp::read_file(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path))
        return std::unexpected{ parse_error_t{ parser_errors::file_not_found, 0 } };

    std::ifstream file{ path };
    if (!file)
        return std::unexpected{ parse_error_t{ parser_errors::file_open_fail, 0 } };

    std::vector<patch_t> result;
    std::string          line;
    std::size_t          size{};
    std::string          filename = path.filename().string();

    while (std::getline(file, line))
    {
        ++size;

        std::expected<patch_t, parser_errors> parsed = read_line(line);

        if (!parsed && parsed.error() == parser_errors::parse_line_empty)
            continue;

        if (!parsed)
            return std::unexpected{ parse_error_t{ parsed.error(), size } };

        parsed->line = size;
        parsed->file = filename;

        result.push_back(std::move(*parsed));
    }

    return result;
}