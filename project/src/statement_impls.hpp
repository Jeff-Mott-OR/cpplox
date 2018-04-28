#pragma once

// Related header
// C standard headers
// C++ standard headers
#include <memory>
#include <vector>
// Third-party headers
// This project's headers
#include "expression.hpp"
#include "statement.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    struct Expr_stmt : Stmt {
        std::shared_ptr<const Expr> expr;

        explicit Expr_stmt(std::shared_ptr<const Expr>&& expr);
        void accept(const std::shared_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct Print_stmt : Stmt {
        std::shared_ptr<const Expr> expr;

        explicit Print_stmt(std::shared_ptr<const Expr>&& expr);
        void accept(const std::shared_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct Var_stmt : Stmt {
        Token name;
        std::shared_ptr<const Expr> initializer;

        explicit Var_stmt(Token&& name, std::shared_ptr<const Expr>&& initializer);
        void accept(const std::shared_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct While_stmt : Stmt {
        std::shared_ptr<const Expr> condition;
        std::shared_ptr<const Stmt> body;

        explicit While_stmt(std::shared_ptr<const Expr>&& condition, std::shared_ptr<const Stmt>&& body);
        void accept(const std::shared_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct Block_stmt : Stmt {
        std::vector<std::shared_ptr<const Stmt>> statements;

        explicit Block_stmt(std::vector<std::shared_ptr<const Stmt>>&& statements);
        void accept(const std::shared_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct If_stmt : Stmt {
        std::shared_ptr<const Expr> condition;
        std::shared_ptr<const Stmt> then_branch;
        std::shared_ptr<const Stmt> else_branch;

        explicit If_stmt(
            std::shared_ptr<const Expr>&& condition,
            std::shared_ptr<const Stmt>&& then_branch,
            std::shared_ptr<const Stmt>&& else_branch
        );
        void accept(const std::shared_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct Function_stmt : Stmt {
        Token name;
        std::vector<Token> parameters;
        std::vector<std::shared_ptr<const Stmt>> body;

        explicit Function_stmt(
            Token&& name,
            std::vector<Token>&& parameters,
            std::vector<std::shared_ptr<const Stmt>>&& body
        );
        void accept(const std::shared_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };

    struct Return_stmt : Stmt {
        std::shared_ptr<const Expr> value;

        explicit Return_stmt(std::shared_ptr<const Expr>&& value);
        void accept(const std::shared_ptr<const Stmt>& owner_this, Stmt_visitor&) const override;
    };
}}
