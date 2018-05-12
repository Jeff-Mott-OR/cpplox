// Related header
#include "expression.hpp"
// C standard headers
// C++ standard headers
// Third-party headers
// This project's headers

using std::shared_ptr;

namespace motts { namespace lox {
    // Most expressions won't be an lvalue, so make that the default
    shared_ptr<const Expr> Expr::make_assignment_expression(
        shared_ptr<const Expr>&& /*lhs_expr*/,
        shared_ptr<const Expr>&& /*rhs_expr*/,
        const Runtime_error& throwable_if_not_lvalue
    ) const {
        throw throwable_if_not_lvalue;
    }
}}
