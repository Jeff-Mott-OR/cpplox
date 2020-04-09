#pragma once

#include <cstddef>
#include <string>

#include <boost/variant.hpp>

#include "function_fwd.hpp"

namespace motts { namespace lox {
    struct Value {
        boost::variant<bool, std::nullptr_t, double, std::string, const Function*, const Native_fn*> variant;
    };

    void print_value(const Value&);
}}
