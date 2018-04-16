#pragma once

// Related header
// C standard headers
// C++ standard headers
#include <stdexcept>
// Third-party headers
// This project's headers

namespace motts { namespace lox {
    struct Runtime_error : std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}}
