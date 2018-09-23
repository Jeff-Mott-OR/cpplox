#pragma once

// Related header
// C standard headers
// C++ standard headers
#include <vector>
// Third-party headers
// This project's headers
#include "expression.hpp"
#include "expression_impls.hpp"
#include "statement.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    struct Expr_stmt : Stmt {
        const Expr* expr {};

        explicit Expr_stmt(const Expr* expr);
        void accept(const Stmt* owner_this, Stmt_visitor&) const override;
    };

    struct Print_stmt : Stmt {
        const Expr* expr {};

        explicit Print_stmt(const Expr* expr);
        void accept(const Stmt* owner_this, Stmt_visitor&) const override;
    };

    struct Var_stmt : Stmt {
        Token name;
        const Expr* initializer {};

        explicit Var_stmt(Token&& name, const Expr* initializer);
        void accept(const Stmt* owner_this, Stmt_visitor&) const override;
    };

    struct While_stmt : Stmt {
        const Expr* condition {};
        const Stmt* body {};

        explicit While_stmt(const Expr* condition, const Stmt* body);
        void accept(const Stmt* owner_this, Stmt_visitor&) const override;
    };

    struct Block_stmt : Stmt {
        std::vector<const Stmt*> statements;

        explicit Block_stmt(std::vector<const Stmt*>&& statements);
        void accept(const Stmt* owner_this, Stmt_visitor&) const override;
    };

    struct If_stmt : Stmt {
        const Expr* condition {};
        const Stmt* then_branch {};
        const Stmt* else_branch {};

        explicit If_stmt(
            const Expr* condition,
            const Stmt* then_branch,
            const Stmt* else_branch
        );
        void accept(const Stmt* owner_this, Stmt_visitor&) const override;
    };

    struct Function_stmt : Stmt {
        Token name;
        std::vector<Token> parameters;
        std::vector<const Stmt*> body;

        explicit Function_stmt(
            Token&& name,
            std::vector<Token>&& parameters,
            std::vector<const Stmt*>&& body
        );
        void accept(const Stmt* owner_this, Stmt_visitor&) const override;
    };

    struct Return_stmt : Stmt {
        Token keyword;
        const Expr* value {};

        explicit Return_stmt(Token&& keyword, const Expr* value);
        void accept(const Stmt* owner_this, Stmt_visitor&) const override;
    };

    struct Class_stmt : Stmt {
        Token name;
        Var_expr* superclass {};
        std::vector<const Function_stmt*> methods;

        explicit Class_stmt(
            Token&& name,
            Var_expr* superclass,
            std::vector<const Function_stmt*>&& methods
        );
        void accept(const Stmt* owner_this, Stmt_visitor&) const override;
    };
}}
