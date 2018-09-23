// Related header
#include "expression.hpp"
// C standard headers
// C++ standard headers
// Third-party headers
// This project's headers

namespace motts { namespace lox {
    // Most expressions won't be an lvalue, so make that the default
    const Expr* Expr::make_assignment_expression(
        const Expr* /*lhs_expr*/,
        const Expr* /*rhs_expr*/,
        const Runtime_error& throwable_if_not_lvalue
    ) const {
        throw throwable_if_not_lvalue;
    }
}}
