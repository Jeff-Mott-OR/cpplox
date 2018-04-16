// Related header
#include "expression.hpp"
// C standard headers
// C++ standard headers
// Third-party headers
// This project's headers

namespace motts { namespace lox {
    // Most expressions won't be an lvalue, so make that the default
    Token&& Expr::lvalue_name(const Runtime_error& throwable_if_not_lvalue) && {
        throw throwable_if_not_lvalue;
    }
}}
