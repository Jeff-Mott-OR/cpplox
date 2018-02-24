#include "expression.hpp"

#include <utility>

using std::move;
using std::unique_ptr;

namespace motts { namespace lox {
    /*
        struct Expr
    */

    Expr::Expr() = default;

    Expr::~Expr() = default;

    /*
        struct Binary_expr
    */

    Binary_expr::Binary_expr(
        unique_ptr<Expr>&& left_arg,
        Token&& op_arg,
        unique_ptr<Expr>&& right_arg
    )
        : left {move(left_arg)},
          op {move(op_arg)},
          right {move(right_arg)}
    {}

    void Binary_expr::accept(Expr_visitor& visitor) const {
        visitor.visit(*this);
    }

    /*
        struct Grouping_expr
    */

    Grouping_expr::Grouping_expr(unique_ptr<Expr>&& expr_arg)
        : expr {move(expr_arg)}
    {}

    void Grouping_expr::accept(Expr_visitor& visitor) const {
        visitor.visit(*this);
    }

    /*
        struct Literal_expr
    */

    Literal_expr::Literal_expr(Literal_multi_type&& value_arg)
        : value {move(value_arg)}
    {}

    void Literal_expr::accept(Expr_visitor& visitor) const {
        visitor.visit(*this);
    }

    /*
        struct Unary_expr
    */

    Unary_expr::Unary_expr(Token&& op_arg, unique_ptr<Expr>&& right_arg)
        : op {move(op_arg)},
          right {move(right_arg)}
    {}

    void Unary_expr::accept(Expr_visitor& visitor) const {
        visitor.visit(*this);
    }

    /*
        struct Expr_visitor
    */

    Expr_visitor::Expr_visitor() = default;

    Expr_visitor::~Expr_visitor() = default;
}}
