#pragma once

#include "expression.hpp"

namespace motts { namespace lox {
    struct Ast_printer : Expr_visitor {
        boost::any visit_binary(const Binary_expr& expr) const override;
        boost::any visit_grouping(const Grouping_expr& expr) const override;
        boost::any visit_literal(const Literal_expr& expr) const override;
        boost::any visit_unary(const Unary_expr& expr) const override;
    };
}}
