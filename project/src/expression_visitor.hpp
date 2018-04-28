#pragma once

// Related header
#include "expression_visitor_fwd.hpp"
// C standard headers
// C++ standard headers
#include <memory>
// Third-party headers
// This project's headers
#include "expression_impls.hpp"

namespace motts { namespace lox {
    struct Expr_visitor {
        virtual void visit(const std::shared_ptr<const Binary_expr>&) = 0;
        virtual void visit(const std::shared_ptr<const Grouping_expr>&) = 0;
        virtual void visit(const std::shared_ptr<const Literal_expr>&) = 0;
        virtual void visit(const std::shared_ptr<const Unary_expr>&) = 0;
        virtual void visit(const std::shared_ptr<const Var_expr>&) = 0;
        virtual void visit(const std::shared_ptr<const Assign_expr>&) = 0;
        virtual void visit(const std::shared_ptr<const Logical_expr>&) = 0;
        virtual void visit(const std::shared_ptr<const Call_expr>&) = 0;

        // Base class boilerplate
        explicit Expr_visitor() = default;
        virtual ~Expr_visitor() = default;
        Expr_visitor(const Expr_visitor&) = delete;
        Expr_visitor& operator=(const Expr_visitor&) = delete;
        Expr_visitor(Expr_visitor&&) = delete;
        Expr_visitor& operator=(Expr_visitor&&) = delete;
    };
}}
