#pragma once

#include <string>
#include <variant>

#include "memory.hpp"
#include "object-fwd.hpp"

namespace motts { namespace lox {
    struct Dynamic_type_value {
        std::variant<std::nullptr_t, bool, double, std::string, GC_ptr<Function>> variant;
    };

    bool operator==(const Dynamic_type_value& lhs, const Dynamic_type_value& rhs);

    std::ostream& operator<<(std::ostream&, const Dynamic_type_value&);
}}
