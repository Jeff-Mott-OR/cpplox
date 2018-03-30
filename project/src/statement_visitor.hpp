#pragma once

// Related header
// C standard headers
// C++ standard headers
// Third-party headers
// This project's headers
#include "statement_impls.hpp"

namespace motts { namespace lox {
    struct Stmt_visitor {
        virtual void visit(const Expr_stmt&) = 0;
        virtual void visit(const Print_stmt&) = 0;
        virtual void visit(const Var_stmt&) = 0;
        virtual void visit(const While_stmt&) = 0;
        virtual void visit(const Block_stmt&) = 0;
        virtual void visit(const If_stmt&) = 0;
        virtual void visit(Function_stmt&) = 0;
        virtual void visit(const Return_stmt&) = 0;

        // Base class boilerplate
        explicit Stmt_visitor() = default;
        virtual ~Stmt_visitor() = default;
        Stmt_visitor(const Stmt_visitor&) = delete;
        Stmt_visitor& operator=(const Stmt_visitor&) = delete;
        Stmt_visitor(Stmt_visitor&&) = delete;
        Stmt_visitor& operator=(Stmt_visitor&&) = delete;
    };
}}
