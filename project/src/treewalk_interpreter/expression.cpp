#include "expression.hpp"

using gcpp::deferred_heap;
using gcpp::deferred_ptr;

namespace motts { namespace lox {
    // Most expressions won't be an lvalue, so make that the default
    deferred_ptr<const Expr> Expr::make_assignment_expression(
        deferred_ptr<const Expr>&& /*lhs_expr*/,
        deferred_ptr<const Expr>&& /*rhs_expr*/,
        deferred_heap&,
        const Parser_error& throwable_if_not_lvalue
    ) const {
        throw throwable_if_not_lvalue;
    }
}}
