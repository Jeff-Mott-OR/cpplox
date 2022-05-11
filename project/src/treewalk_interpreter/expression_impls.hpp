#pragma once

#include <vector>

#include <boost/optional.hpp>

#include "expression.hpp"
#include "literal.hpp"
#include "statement.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    struct Binary_expr : Expr {
        gcpp::deferred_ptr<const Expr> left;
        Token op;
        gcpp::deferred_ptr<const Expr> right;

        explicit Binary_expr(gcpp::deferred_ptr<const Expr>&& left, Token&& op, gcpp::deferred_ptr<const Expr>&& right);
        void accept(const gcpp::deferred_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Grouping_expr : Expr {
        gcpp::deferred_ptr<const Expr> expr;

        explicit Grouping_expr(gcpp::deferred_ptr<const Expr>&& expr);
        void accept(const gcpp::deferred_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Literal_expr : Expr {
        Literal value;

        explicit Literal_expr(Literal&& value);
        void accept(const gcpp::deferred_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Unary_expr : Expr {
        Token op;
        gcpp::deferred_ptr<const Expr> right;

        explicit Unary_expr(Token&& op, gcpp::deferred_ptr<const Expr>&& right);
        void accept(const gcpp::deferred_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Var_expr : Expr {
        Token name;

        explicit Var_expr(Token&& name);
        void accept(const gcpp::deferred_ptr<const Expr>& owner_this, Expr_visitor&) const override;
        gcpp::deferred_ptr<const Expr> make_assignment_expression(
            gcpp::deferred_ptr<const Expr>&& lhs_expr,
            gcpp::deferred_ptr<const Expr>&& rhs_expr,
            gcpp::deferred_heap&,
            const Parser_error& throwable_if_not_lvalue
        ) const override;
    };

    struct Assign_expr : Expr {
        Token name;
        gcpp::deferred_ptr<const Expr> value;

        explicit Assign_expr(Token&& name, gcpp::deferred_ptr<const Expr>&& value);
        void accept(const gcpp::deferred_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Logical_expr : Expr {
        gcpp::deferred_ptr<const Expr> left;
        Token op;
        gcpp::deferred_ptr<const Expr> right;

        explicit Logical_expr(gcpp::deferred_ptr<const Expr>&& left, Token&& op, gcpp::deferred_ptr<const Expr>&& right);
        void accept(const gcpp::deferred_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Call_expr : Expr {
        gcpp::deferred_ptr<const Expr> callee;
        Token closing_paren;
        std::vector<gcpp::deferred_ptr<const Expr>> arguments;

        explicit Call_expr(
            gcpp::deferred_ptr<const Expr>&& callee,
            Token&& closing_paren,
            std::vector<gcpp::deferred_ptr<const Expr>>&& arguments
        );
        void accept(const gcpp::deferred_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Get_expr : Expr {
        gcpp::deferred_ptr<const Expr> object;
        Token name;

        explicit Get_expr(gcpp::deferred_ptr<const Expr>&& object, Token&& name);
        void accept(const gcpp::deferred_ptr<const Expr>& owner_this, Expr_visitor&) const override;
        gcpp::deferred_ptr<const Expr> make_assignment_expression(
            gcpp::deferred_ptr<const Expr>&& lhs_expr,
            gcpp::deferred_ptr<const Expr>&& rhs_expr,
            gcpp::deferred_heap&,
            const Parser_error& throwable_if_not_lvalue
        ) const override;
    };

    struct Set_expr : Expr {
        gcpp::deferred_ptr<const Expr> object;
        Token name;
        gcpp::deferred_ptr<const Expr> value;

        explicit Set_expr(gcpp::deferred_ptr<const Expr>&& object, Token&& name, gcpp::deferred_ptr<const Expr>&& value);
        void accept(const gcpp::deferred_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct This_expr : Expr {
        Token keyword;

        explicit This_expr(Token&& keyword);
        void accept(const gcpp::deferred_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Super_expr : Expr {
        Token keyword;
        Token method;

        explicit Super_expr(Token&& keyword, Token&& method);
        void accept(const gcpp::deferred_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };

    struct Function_expr : Expr {
        boost::optional<Token> name;
        std::vector<Token> parameters;
        std::vector<gcpp::deferred_ptr<const Stmt>> body;

        explicit Function_expr(
            boost::optional<Token>&& name,
            std::vector<Token>&& parameters,
            std::vector<gcpp::deferred_ptr<const Stmt>>&& body
        );
        void accept(const gcpp::deferred_ptr<const Expr>& owner_this, Expr_visitor&) const override;
    };
}}
