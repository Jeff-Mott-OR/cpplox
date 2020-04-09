#pragma once

#include <stdexcept>
#include <string>

#include "function.hpp"
#include "scanner.hpp"

namespace motts { namespace lox {
    Function compile(const std::string& source);

    struct Compiler_error : std::runtime_error {
        using std::runtime_error::runtime_error;
        Compiler_error(const Token&, const std::string& what);
    };
}}
