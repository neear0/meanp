#include "error_handler.hpp"

const char* meanp::parser::error_category_t::name() const noexcept
{
	return "meanp_parser";
}

std::string meanp::parser::error_category_t::message(int c) const
{
	switch (static_cast<parser_errors>(c))
	{
	case parser_errors::file_not_found:              return "input file does not exist";
	case parser_errors::file_open_fail:              return "input file could not be opened";
	case parser_errors::parse_line_empty:            return "line is empty or a comment";
	case parser_errors::parse_insufficient_args:     return "line does not contain enough arguments";
	case parser_errors::parse_too_many_args:         return "line contains too many arguments";
	case parser_errors::parse_target_unclosed_quote: return "target filename contains an unclosed quote";
	case parser_errors::parse_bad_offset_address:    return "offset is not a valid hexadecimal number";
	case parser_errors::parse_bad_data_length:       return "data length is not a multiple of 2";
	case parser_errors::parse_bad_data_bytes:        return "data contains invalid hex characters";
	}
	return "unknown parser error";
}

std::error_code meanp::parser::make_error_code(parser_errors e)
{
	static const error_category_t instance{};
	return { static_cast<int>(e), instance };
}
