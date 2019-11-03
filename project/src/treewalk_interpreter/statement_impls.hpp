#pragma once

#include <vector>

#include "expression.hpp"
#include "expression_impls.hpp"
#include "statement.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    struct Expr_stmt : Stmt {
        gcpp::deferred_ptr<const Expr> expr;

        explicit Expr_stmt(gcpp::deferred_ptr<const Expr>&& expr);
        void accept(const gcpp::deferred_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct Print_stmt : Stmt {
        gcpp::deferred_ptr<const Expr> expr;

        explicit Print_stmt(gcpp::deferred_ptr<const Expr>&& expr);
        void accept(const gcpp::deferred_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct Var_stmt : Stmt {
        Token name;
        gcpp::deferred_ptr<const Expr> initializer;

        explicit Var_stmt(Token&& name, gcpp::deferred_ptr<const Expr>&& initializer);
        void accept(const gcpp::deferred_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct While_stmt : Stmt {
        gcpp::deferred_ptr<const Expr> condition;
        gcpp::deferred_ptr<const Stmt> body;

        explicit While_stmt(gcpp::deferred_ptr<const Expr>&& condition, gcpp::deferred_ptr<const Stmt>&& body);
        void accept(const gcpp::deferred_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct For_stmt : Stmt {
        gcpp::deferred_ptr<const Expr> condition;
        gcpp::deferred_ptr<const Expr> increment;
        gcpp::deferred_ptr<const Stmt> body;

        explicit For_stmt(gcpp::deferred_ptr<const Expr>&& condition, gcpp::deferred_ptr<const Expr>&& increment, gcpp::deferred_ptr<const Stmt>&& body);
        void accept(const gcpp::deferred_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct Block_stmt : Stmt {
        std::vector<gcpp::deferred_ptr<const Stmt>> statements;

        explicit Block_stmt(std::vector<gcpp::deferred_ptr<const Stmt>>&& statements);
        void accept(const gcpp::deferred_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct If_stmt : Stmt {
        gcpp::deferred_ptr<const Expr> condition;
        gcpp::deferred_ptr<const Stmt> then_branch;
        gcpp::deferred_ptr<const Stmt> else_branch;

        explicit If_stmt(
            gcpp::deferred_ptr<const Expr>&& condition,
            gcpp::deferred_ptr<const Stmt>&& then_branch,
            gcpp::deferred_ptr<const Stmt>&& else_branch
        );
        void accept(const gcpp::deferred_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct Function_stmt : Stmt {
        gcpp::deferred_ptr<const Function_expr> expr;

        explicit Function_stmt(gcpp::deferred_ptr<const Function_expr>&& expr);
        void accept(const gcpp::deferred_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct Return_stmt : Stmt {
        Token keyword;
        gcpp::deferred_ptr<const Expr> value;

        explicit Return_stmt(Token&& keyword, gcpp::deferred_ptr<const Expr>&& value);
        void accept(const gcpp::deferred_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct Class_stmt : Stmt {
        Token name;
        gcpp::deferred_ptr<const Var_expr> superclass;
        std::vector<gcpp::deferred_ptr<const Function_stmt>> methods;

        explicit Class_stmt(
            Token&& name,
            gcpp::deferred_ptr<const Var_expr>&& superclass,
            std::vector<gcpp::deferred_ptr<const Function_stmt>>&& methods
        );
        void accept(const gcpp::deferred_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct Break_stmt : Stmt {
        explicit Break_stmt();
        void accept(const gcpp::deferred_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct Continue_stmt : Stmt {
        explicit Continue_stmt();
        void accept(const gcpp::deferred_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };
}}
