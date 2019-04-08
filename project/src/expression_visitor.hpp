#pragma once

#pragma warning(push, 0)
    #include <deferred_heap.h>
#pragma warning(pop)

#include "expression_visitor_fwd.hpp"
#include "expression_impls.hpp"

namespace motts { namespace lox {
    struct Expr_visitor {
        virtual void visit(const gcpp::deferred_ptr<const Binary_expr>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Grouping_expr>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Literal_expr>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Unary_expr>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Var_expr>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Assign_expr>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Logical_expr>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Call_expr>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Get_expr>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Set_expr>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const This_expr>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Super_expr>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Function_expr>&) = 0;

        // Base class boilerplate
        explicit Expr_visitor() = default;
        virtual ~Expr_visitor() = default;
        Expr_visitor(const Expr_visitor&) = delete;
        Expr_visitor& operator=(const Expr_visitor&) = delete;
        Expr_visitor(Expr_visitor&&) = delete;
        Expr_visitor& operator=(Expr_visitor&&) = delete;
    };
}}
