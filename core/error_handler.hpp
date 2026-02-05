#pragma once

#include <string>
#include <system_error>

namespace meanp::parser
{
    enum class parser_errors : int
    {
        file_not_found = 1,
        file_open_fail,
        parse_line_empty,
        parse_insufficient_args,
        parse_too_many_args,
        parse_target_unclosed_quote,
        parse_bad_offset_address,
        parse_bad_data_length,
        parse_bad_data_bytes,
    };

    struct error_category_t final : std::error_category
    {
        const char* name()         const noexcept override;
        std::string message(int c) const override;
    };

    std::error_code make_error_code(parser_errors e);

} 

template <>
struct std::is_error_code_enum<meanp::parser::parser_errors> : std::true_type {};