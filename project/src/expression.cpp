#include "expression.hpp"

#include <utility>

using std::move;
using std::unique_ptr;

namespace motts { namespace lox {
    // struct Expr

        Expr::Expr() = default;

        Expr::~Expr() = default;

    // struct Binary_expr

        Binary_expr::Binary_expr(
            unique_ptr<Expr>&& left_,
            Token&& op_,
            unique_ptr<Expr>&& right_
        )
            : left {move(left_)},
              op {move(op_)},
              right {move(right_)}
        {}

        void Binary_expr::accept(Expr_visitor& visitor) const {
            visitor.visit(*this);
        }

    // struct Grouping_expr

        Grouping_expr::Grouping_expr(unique_ptr<Expr>&& expr_)
            : expr {move(expr_)}
        {}

        void Grouping_expr::accept(Expr_visitor& visitor) const {
            visitor.visit(*this);
        }

    // struct Literal_expr

        Literal_expr::Literal_expr(Literal_multi_type&& value_)
            : value {move(value_)}
        {}

        void Literal_expr::accept(Expr_visitor& visitor) const {
            visitor.visit(*this);
        }

     // struct Unary_expr

        Unary_expr::Unary_expr(Token&& op_, unique_ptr<Expr>&& right_)
            : op {move(op_)},
              right {move(right_)}
        {}

        void Unary_expr::accept(Expr_visitor& visitor) const {
            visitor.visit(*this);
        }

    // struct Expr_visitor

        Expr_visitor::Expr_visitor() = default;

        Expr_visitor::~Expr_visitor() = default;
}}
