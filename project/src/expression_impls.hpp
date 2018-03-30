#pragma once

// Related header
// C standard headers
// C++ standard headers
#include <memory>
// Third-party headers
// This project's headers
#include "expression.hpp"
#include "literal.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    struct Binary_expr : Expr {
        std::unique_ptr<Expr> left;
        Token op;
        std::unique_ptr<Expr> right;

        explicit Binary_expr(std::unique_ptr<Expr>&& left, Token&& op, std::unique_ptr<Expr>&& right);
        void accept(Expr_visitor&) const override;
    };

    struct Grouping_expr : Expr {
        std::unique_ptr<Expr> expr;

        explicit Grouping_expr(std::unique_ptr<Expr>&& expr);
        void accept(Expr_visitor&) const override;
    };

    struct Literal_expr : Expr {
        Literal value;

        explicit Literal_expr(Literal&& value);
        void accept(Expr_visitor&) const override;
    };

    struct Unary_expr : Expr {
        Token op;
        std::unique_ptr<Expr> right;

        explicit Unary_expr(Token&& op, std::unique_ptr<Expr>&& right);
        void accept(Expr_visitor&) const override;
    };

    struct Var_expr : Expr {
        Token name;

        explicit Var_expr(Token&& name);
        void accept(Expr_visitor&) const override;
    };

    struct Assign_expr : Expr {
        Token name;
        std::unique_ptr<Expr> value;

        explicit Assign_expr(Token&& name, std::unique_ptr<Expr>&& value);
        void accept(Expr_visitor&) const override;
    };

    struct Logical_expr : Expr {
        std::unique_ptr<Expr> left;
        Token op;
        std::unique_ptr<Expr> right;

        explicit Logical_expr(std::unique_ptr<Expr>&& left, Token&& op, std::unique_ptr<Expr>&& right);
        void accept(Expr_visitor&) const override;
    };
}}
