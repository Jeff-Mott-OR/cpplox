#pragma once

#include <stdexcept>

namespace motts { namespace lox {
    struct Runtime_error : std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}}
