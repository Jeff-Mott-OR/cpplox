#pragma once

#include <cstddef>
#include <boost/variant.hpp>

namespace motts { namespace lox {
    struct Value {
        boost::variant<bool, std::nullptr_t, double> variant;
    };

    void print_value(Value);
}}
