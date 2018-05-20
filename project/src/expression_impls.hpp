#pragma once

// Related header
// C standard headers
// C++ standard headers
#include <memory>
#include <vector>
// Third-party headers
// This project's headers
#include "expression.hpp"
#include "literal.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    struct Binary_expr : Expr {
        std::shared_ptr<const Expr> left;
        Token op;
        std::shared_ptr<const Expr> right;

        explicit Binary_expr(std::shared_ptr<const Expr>&& left, Token&& op, std::shared_ptr<const Expr>&& right);
        void accept(const std::shared_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Grouping_expr : Expr {
        std::shared_ptr<const Expr> expr;

        explicit Grouping_expr(std::shared_ptr<const Expr>&& expr);
        void accept(const std::shared_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Literal_expr : Expr {
        Literal value;

        explicit Literal_expr(Literal&& value);
        void accept(const std::shared_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Unary_expr : Expr {
        Token op;
        std::shared_ptr<const Expr> right;

        explicit Unary_expr(Token&& op, std::shared_ptr<const Expr>&& right);
        void accept(const std::shared_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Var_expr : Expr {
        Token name;

        explicit Var_expr(Token&& name);
        void accept(const std::shared_ptr<const Expr>& owner_this, Expr_visitor&) const override;
        std::shared_ptr<const Expr> make_assignment_expression(
            std::shared_ptr<const Expr>&& lhs_expr,
            std::shared_ptr<const Expr>&& rhs_expr,
            const Runtime_error& throwable_if_not_lvalue
        ) const override;
    };

    struct Assign_expr : Expr {
        Token name;
        std::shared_ptr<const Expr> value;

        explicit Assign_expr(Token&& name, std::shared_ptr<const Expr>&& value);
        void accept(const std::shared_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Logical_expr : Expr {
        std::shared_ptr<const Expr> left;
        Token op;
        std::shared_ptr<const Expr> right;

        explicit Logical_expr(std::shared_ptr<const Expr>&& left, Token&& op, std::shared_ptr<const Expr>&& right);
        void accept(const std::shared_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Call_expr : Expr {
        std::shared_ptr<const Expr> callee;
        Token closing_paren;
        std::vector<std::shared_ptr<const Expr>> arguments;

        explicit Call_expr(
            std::shared_ptr<const Expr>&& callee,
            Token&& closing_paren,
            std::vector<std::shared_ptr<const Expr>>&& arguments
        );
        void accept(const std::shared_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Get_expr : Expr {
        std::shared_ptr<const Expr> object;
        Token name;

        explicit Get_expr(std::shared_ptr<const Expr>&& object, Token&& name);
        void accept(const std::shared_ptr<const Expr>& owner_this, Expr_visitor&) const override;
        std::shared_ptr<const Expr> make_assignment_expression(
            std::shared_ptr<const Expr>&& lhs_expr,
            std::shared_ptr<const Expr>&& rhs_expr,
            const Runtime_error& throwable_if_not_lvalue
        ) const override;
    };

    struct Set_expr : Expr {
        std::shared_ptr<const Expr> object;
        Token name;
        std::shared_ptr<const Expr> value;

        explicit Set_expr(std::shared_ptr<const Expr>&& object, Token&& name, std::shared_ptr<const Expr>&& value);
        void accept(const std::shared_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct This_expr : Expr {
        Token keyword;

        explicit This_expr(Token&& keyword);
        void accept(const std::shared_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Super_expr : Expr {
        Token keyword;
        Token method;

        explicit Super_expr(Token&& keyword, Token&& method);
        void accept(const std::shared_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };
}}
