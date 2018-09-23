#pragma once

// Related header
#include "expression_visitor_fwd.hpp"
// C standard headers
// C++ standard headers
// Third-party headers
// This project's headers
#include "expression_impls.hpp"

namespace motts { namespace lox {
    struct Expr_visitor {
        virtual void visit(const Binary_expr*) = 0;
        virtual void visit(const Grouping_expr*) = 0;
        virtual void visit(const Literal_expr*) = 0;
        virtual void visit(const Unary_expr*) = 0;
        virtual void visit(const Var_expr*) = 0;
        virtual void visit(const Assign_expr*) = 0;
        virtual void visit(const Logical_expr*) = 0;
        virtual void visit(const Call_expr*) = 0;
        virtual void visit(const Get_expr*) = 0;
        virtual void visit(const Set_expr*) = 0;
        virtual void visit(const This_expr*) = 0;
        virtual void visit(const Super_expr*) = 0;

        // Base class boilerplate
        explicit Expr_visitor() = default;
        virtual ~Expr_visitor() = default;
        Expr_visitor(const Expr_visitor&) = delete;
        Expr_visitor& operator=(const Expr_visitor&) = delete;
        Expr_visitor(Expr_visitor&&) = delete;
        Expr_visitor& operator=(Expr_visitor&&) = delete;
    };
}}
