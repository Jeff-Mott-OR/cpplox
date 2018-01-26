#include "expression.hpp"

#include <utility>

using std::move;
using std::unique_ptr;

using boost::any;

namespace motts { namespace lox {
    // struct Expr

        Expr::Expr() = default;

        Expr::~Expr() = default;

    // struct Binary_expr

        Binary_expr::Binary_expr(
            unique_ptr<Expr>&& left_,
            unique_ptr<Token>&& op_,
            unique_ptr<Expr>&& right_
        )
            : left {move(left_)},
              op {move(op_)},
              right {move(right_)}
        {}

        any Binary_expr::accept(const Expr_visitor& visitor) const {
            return visitor.visit_binary(*this);
        }

    // struct Grouping_expr

        Grouping_expr::Grouping_expr(unique_ptr<Expr>&& expr_)
            : expr {move(expr_)}
        {}

        any Grouping_expr::accept(const Expr_visitor& visitor) const {
            return visitor.visit_grouping(*this);
        }

    // struct Literal_expr

        Literal_expr::Literal_expr() = default;

        any Literal_expr::accept(const Expr_visitor& visitor) const {
            return visitor.visit_literal(*this);
        }

     // struct Unary_expr

        Unary_expr::Unary_expr(unique_ptr<Token>&& op_, unique_ptr<Expr>&& right_)
            : op {move(op_)},
              right {move(right_)}
        {}

        any Unary_expr::accept(const Expr_visitor& visitor) const {
            return visitor.visit_unary(*this);
        }

    // struct Expr_visitor

        Expr_visitor::Expr_visitor() = default;

        Expr_visitor::~Expr_visitor() = default;
}}
