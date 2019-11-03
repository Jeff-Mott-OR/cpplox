#pragma once

#include <stdexcept>
#include <string>

#include "chunk.hpp"
#include "scanner.hpp"

namespace motts { namespace lox {
    Chunk compile(const std::string& source);

    struct Compiler_error : std::runtime_error {
        Compiler_error(const Token&, const std::string& what);
    };
}}
