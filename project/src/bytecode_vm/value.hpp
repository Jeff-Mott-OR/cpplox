#pragma once

#include <cstddef>
#include <string>

#include <boost/variant.hpp>

namespace motts { namespace lox {
    struct Value {
        boost::variant<bool, std::nullptr_t, double, std::string> variant;
    };

    void print_value(const Value&);
}}
