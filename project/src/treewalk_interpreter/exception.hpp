#pragma once

#include <stdexcept>
#include "token.hpp"

namespace motts { namespace lox {
    struct Runtime_error : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    struct Parser_error : Runtime_error {
        explicit Parser_error(const std::string& what, const Token&);
    };
}}
