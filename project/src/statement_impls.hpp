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
        std::unique_ptr<Expr> expr;

        explicit Expr_stmt(std::unique_ptr<Expr>&& expr);
        void accept(Stmt_visitor&) override;
        std::unique_ptr<Stmt> clone() const override;
    };

    struct Print_stmt : Stmt {
        std::unique_ptr<Expr> expr;

        explicit Print_stmt(std::unique_ptr<Expr>&& expr);
        void accept(Stmt_visitor&) override;
        std::unique_ptr<Stmt> clone() const override;
    };

    struct Var_stmt : Stmt {
        Token name;
        std::unique_ptr<Expr> initializer;

        explicit Var_stmt(Token&& name, std::unique_ptr<Expr>&& initializer);
        void accept(Stmt_visitor&) override;
        std::unique_ptr<Stmt> clone() const override;
    };

    struct While_stmt : Stmt {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Stmt> body;

        explicit While_stmt(std::unique_ptr<Expr>&& condition, std::unique_ptr<Stmt>&& body);
        void accept(Stmt_visitor&) override;
        std::unique_ptr<Stmt> clone() const override;
    };

    struct Block_stmt : Stmt {
        std::vector<std::unique_ptr<Stmt>> statements;

        explicit Block_stmt(std::vector<std::unique_ptr<Stmt>>&& statements);
        void accept(Stmt_visitor&) override;
        std::unique_ptr<Stmt> clone() const override;
    };

    struct If_stmt : Stmt {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Stmt> then_branch;
        std::unique_ptr<Stmt> else_branch;

        explicit If_stmt(
            std::unique_ptr<Expr>&& condition,
            std::unique_ptr<Stmt>&& then_branch,
            std::unique_ptr<Stmt>&& else_branch
        );
        void accept(Stmt_visitor&) override;
        std::unique_ptr<Stmt> clone() const override;
    };

    struct Function_stmt : Stmt {
        Token name;
        std::vector<Token> parameters;
        std::vector<std::unique_ptr<Stmt>> body;

        explicit Function_stmt(
            Token&& name,
            std::vector<Token>&& parameters,
            std::vector<std::unique_ptr<Stmt>>&& body
        );
        explicit Function_stmt(const Function_stmt&);
        void accept(Stmt_visitor&) override;
        std::unique_ptr<Stmt> clone() const override;
    };

    struct Return_stmt : Stmt {
        Token keyword;
        std::unique_ptr<Expr> value;

        explicit Return_stmt(Token&& keyword, std::unique_ptr<Expr>&& value);
        void accept(Stmt_visitor&) override;
        std::unique_ptr<Stmt> clone() const override;
    };
}}
