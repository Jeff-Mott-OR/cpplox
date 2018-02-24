#pragma once

#include <memory>
#include <vector>

#include "expression.hpp"

namespace motts { namespace lox {
    struct Stmt_visitor;

    struct Stmt {
        explicit Stmt();

        virtual void accept(Stmt_visitor&) const = 0;

        // Base class boilerplate
        virtual ~Stmt();
        Stmt(const Stmt&) = delete;
        Stmt& operator=(const Stmt&) = delete;
        Stmt(Stmt&&) = delete;
        Stmt& operator=(Stmt&&) = delete;
    };

    struct Expr_stmt : Stmt {
        std::unique_ptr<Expr> expr;

        explicit Expr_stmt(std::unique_ptr<Expr>&& expr);
        void accept(Stmt_visitor&) const override;
    };

    struct Print_stmt : Stmt {
        std::unique_ptr<Expr> expr;

        explicit Print_stmt(std::unique_ptr<Expr>&& expr);
        void accept(Stmt_visitor&) const override;
    };

    struct Var_stmt : Stmt {
        Token name;
        std::unique_ptr<Expr> initializer;

        explicit Var_stmt(Token&& name, std::unique_ptr<Expr>&& initializer);
        void accept(Stmt_visitor&) const override;
    };

    struct Block_stmt : Stmt {
        std::vector<std::unique_ptr<Stmt>> statements;

        Block_stmt(std::vector<std::unique_ptr<Stmt>>&& statements);
        void accept(Stmt_visitor&) const override;
    };

    struct Stmt_visitor {
        explicit Stmt_visitor();

        virtual void visit(const Expr_stmt&) = 0;
        virtual void visit(const Print_stmt&) = 0;
        virtual void visit(const Var_stmt&) = 0;
        virtual void visit(const Block_stmt&) = 0;

        // Base class boilerplate
        virtual ~Stmt_visitor();
        Stmt_visitor(const Stmt_visitor&) = delete;
        Stmt_visitor& operator=(const Stmt_visitor&) = delete;
        Stmt_visitor(Stmt_visitor&&) = delete;
        Stmt_visitor& operator=(Stmt_visitor&&) = delete;
    };
}}
