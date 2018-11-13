#pragma once

#include <vector>

#include "expression.hpp"
#include "literal.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    struct Binary_expr : Expr {
        const Expr* left {};
        Token op;
        const Expr* right {};

        explicit Binary_expr(const Expr* left, Token&& op, const Expr* right);
        void accept(const Expr* owner_this, Expr_visitor&) const override;
    };

    struct Grouping_expr : Expr {
        const Expr* expr {};

        explicit Grouping_expr(const Expr* expr);
        void accept(const Expr* owner_this, Expr_visitor&) const override;
    };

    struct Literal_expr : Expr {
        Literal value;

        explicit Literal_expr(Literal&& value);
        void accept(const Expr* owner_this, Expr_visitor&) const override;
    };

    struct Unary_expr : Expr {
        Token op;
        const Expr* right {};

        explicit Unary_expr(Token&& op, const Expr* right);
        void accept(const Expr* owner_this, Expr_visitor&) const override;
    };

    struct Var_expr : Expr {
        Token name;

        explicit Var_expr(Token&& name);
        void accept(const Expr* owner_this, Expr_visitor&) const override;
        const Expr* make_assignment_expression(
            const Expr* lhs_expr,
            const Expr* rhs_expr,
            const Runtime_error& throwable_if_not_lvalue
        ) const override;
    };

    struct Assign_expr : Expr {
        Token name;
        const Expr* value {};

        explicit Assign_expr(Token&& name, const Expr* value);
        void accept(const Expr* owner_this, Expr_visitor&) const override;
    };

    struct Logical_expr : Expr {
        const Expr* left {};
        Token op;
        const Expr* right {};

        explicit Logical_expr(const Expr* left, Token&& op, const Expr* right);
        void accept(const Expr* owner_this, Expr_visitor&) const override;
    };

    struct Call_expr : Expr {
        const Expr* callee {};
        Token closing_paren;
        std::vector<const Expr*> arguments;

        explicit Call_expr(
            const Expr* callee,
            Token&& closing_paren,
            std::vector<const Expr*>&& arguments
        );
        void accept(const Expr* owner_this, Expr_visitor&) const override;
    };

    struct Get_expr : Expr {
        const Expr* object {};
        Token name;

        explicit Get_expr(const Expr* object, Token&& name);
        void accept(const Expr* owner_this, Expr_visitor&) const override;
        const Expr* make_assignment_expression(
            const Expr* lhs_expr,
            const Expr* rhs_expr,
            const Runtime_error& throwable_if_not_lvalue
        ) const override;
    };

    struct Set_expr : Expr {
        const Expr* object {};
        Token name;
        const Expr* value {};

        explicit Set_expr(const Expr* object, Token&& name, const Expr* value);
        void accept(const Expr* owner_this, Expr_visitor&) const override;
    };

    struct This_expr : Expr {
        Token keyword;

        explicit This_expr(Token&& keyword);
        void accept(const Expr* owner_this, Expr_visitor&) const override;
    };

    struct Super_expr : Expr {
        Token keyword;
        Token method;

        explicit Super_expr(Token&& keyword, Token&& method);
        void accept(const Expr* owner_this, Expr_visitor&) const override;
    };
}}
