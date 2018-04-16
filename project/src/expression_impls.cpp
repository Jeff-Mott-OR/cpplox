// Related header
#include "expression_impls.hpp"
// C standard headers
// C++ standard headers
#include <algorithm>
#include <iterator>
#include <utility>
// Third-party headers
// This project's headers
#include "expression_visitor.hpp"

using std::back_inserter;
using std::make_unique;
using std::move;
using std::transform;
using std::unique_ptr;
using std::vector;

namespace motts { namespace lox {
    /*
        struct Binary_expr
    */

    Binary_expr::Binary_expr(unique_ptr<Expr>&& left_arg, Token&& op_arg, unique_ptr<Expr>&& right_arg) :
        left {move(left_arg)},
        op {move(op_arg)},
        right {move(right_arg)}
    {}

    void Binary_expr::accept(Expr_visitor& visitor) const {
        visitor.visit(*this);
    }

    unique_ptr<Expr> Binary_expr::clone() const {
        return make_unique<Binary_expr>(left->clone(), Token{op}, right->clone());
    }

    /*
        struct Grouping_expr
    */

    Grouping_expr::Grouping_expr(unique_ptr<Expr>&& expr_arg) :
        expr {move(expr_arg)}
    {}

    void Grouping_expr::accept(Expr_visitor& visitor) const {
        visitor.visit(*this);
    }

    unique_ptr<Expr> Grouping_expr::clone() const {
        return make_unique<Grouping_expr>(expr->clone());
    }

    /*
        struct Literal_expr
    */

    Literal_expr::Literal_expr(Literal&& value_arg) :
        value {move(value_arg)}
    {}

    void Literal_expr::accept(Expr_visitor& visitor) const {
        visitor.visit(*this);
    }

    unique_ptr<Expr> Literal_expr::clone() const {
        return make_unique<Literal_expr>(Literal{value});
    }

    /*
        struct Unary_expr
    */

    Unary_expr::Unary_expr(Token&& op_arg, unique_ptr<Expr>&& right_arg) :
        op {move(op_arg)},
        right {move(right_arg)}
    {}

    void Unary_expr::accept(Expr_visitor& visitor) const {
        visitor.visit(*this);
    }

    unique_ptr<Expr> Unary_expr::clone() const {
        return make_unique<Unary_expr>(Token{op}, right->clone());
    }

    /*
        struct Var_expr
    */

    Var_expr::Var_expr(Token&& name_arg) :
        name {move(name_arg)}
    {}

    void Var_expr::accept(Expr_visitor& visitor) const {
        visitor.visit(*this);
    }

    unique_ptr<Expr> Var_expr::clone() const {
        return make_unique<Var_expr>(Token{name});
    }

    /*
        struct Assign_expr
    */

    Assign_expr::Assign_expr(Token&& name_arg, std::unique_ptr<Expr>&& value_arg) :
        name {move(name_arg)},
        value {move(value_arg)}
    {}

    void Assign_expr::accept(Expr_visitor& visitor) const {
        visitor.visit(*this);
    }

    unique_ptr<Expr> Assign_expr::clone() const {
        return make_unique<Assign_expr>(Token{name}, value->clone());
    }

    /*
        struct Logical_expr
    */

    Logical_expr::Logical_expr(unique_ptr<Expr>&& left_arg, Token&& op_arg, unique_ptr<Expr>&& right_arg) :
        left {move(left_arg)},
        op {move(op_arg)},
        right {move(right_arg)}
    {}

    void Logical_expr::accept(Expr_visitor& visitor) const {
        visitor.visit(*this);
    }

    unique_ptr<Expr> Logical_expr::clone() const {
        return make_unique<Logical_expr>(left->clone(), Token{op}, right->clone());
    }

    /*
        struct Call_expr
    */

    Call_expr::Call_expr(
        unique_ptr<Expr>&& callee_arg,
        Token&& closing_paren_arg,
        vector<unique_ptr<Expr>>&& arguments_arg
    ) :
        callee {move(callee_arg)},
        closing_paren {move(closing_paren_arg)},
        arguments {move(arguments_arg)}
    {}

    void Call_expr::accept(Expr_visitor& visitor) const {
        visitor.visit(*this);
    }

    unique_ptr<Expr> Call_expr::clone() const {
        vector<unique_ptr<Expr>> arguments_copy;
        transform(arguments.cbegin(), arguments.cend(), back_inserter(arguments_copy), [] (const auto& expr) {
            return expr->clone();
        });

        return make_unique<Call_expr>(callee->clone(), Token{closing_paren}, move(arguments_copy));
    }
}}
