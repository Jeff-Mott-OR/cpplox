#pragma once

#pragma warning(push, 0)
    #include <deferred_heap.h>
#pragma warning(pop)

#include "statement_visitor_fwd.hpp"
#include "statement_impls.hpp"

namespace motts { namespace lox {
    struct Stmt_visitor {
        virtual void visit(const gcpp::deferred_ptr<const Expr_stmt>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Print_stmt>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Var_stmt>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const While_stmt>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const For_stmt>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Block_stmt>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const If_stmt>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Function_stmt>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Return_stmt>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Class_stmt>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Break_stmt>&) = 0;
        virtual void visit(const gcpp::deferred_ptr<const Continue_stmt>&) = 0;

        // Base class boilerplate
        explicit Stmt_visitor() = default;
        virtual ~Stmt_visitor() = default;
        Stmt_visitor(const Stmt_visitor&) = delete;
        Stmt_visitor& operator=(const Stmt_visitor&) = delete;
        Stmt_visitor(Stmt_visitor&&) = delete;
        Stmt_visitor& operator=(Stmt_visitor&&) = delete;
    };
}}
