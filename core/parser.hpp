// ReSharper disable All
#pragma once
#include <expected>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "error_handler.hpp"

namespace meanp::parser
{

    enum class addr_type : std::uint8_t
    {
        absolute,   
        rva,        
        file,       
    };

    struct patch_t
    {
        addr_type                 type{};       
        std::uintptr_t            address{};    
        std::size_t               line{};       
        std::string               file;         
        std::string               target;       
        std::vector<std::uint8_t> on;           
        std::vector<std::uint8_t> off;          

        std::string_view type_name()   const;
        std::string      target_name() const;
    };

    struct parse_error_t
    {
        parser_errors ec;
        std::size_t   line;     
    };

    struct read_target_result_t
    {
        std::string            name;
        std::string::size_type offset; 
    };

    struct read_offset_result_t
    {
        addr_type      type;
        std::uintptr_t address;
    };

    std::expected<meanp::parser::read_target_result_t, meanp::parser::parser_errors> read_target(std::string_view line);
    std::expected<meanp::parser::read_offset_result_t, meanp::parser::parser_errors> read_offset(std::string_view token);
    std::expected<std::vector<std::uint8_t>, meanp::parser::parser_errors> read_data(std::string_view hex);
    std::expected<meanp::parser::patch_t, meanp::parser::parser_errors> read_line(std::string_view line);
    std::expected<std::vector<patch_t>, parse_error_t> read_file(const std::filesystem::path& path);

}
