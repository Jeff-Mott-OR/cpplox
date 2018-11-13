#include "expression_impls.hpp"
#include <utility>
#include <gc.h>
#include "expression_visitor.hpp"

using std::move;
using std::vector;

namespace motts { namespace lox {
    /*
        struct Binary_expr
    */

    Binary_expr::Binary_expr(const Expr* left_arg, Token&& op_arg, const Expr* right_arg) :
        left {move(left_arg)},
        op {move(op_arg)},
        right {move(right_arg)}
    {}

    void Binary_expr::accept(const Expr* owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_cast<const Binary_expr*>(owner_this));
    }

    /*
        struct Grouping_expr
    */

    Grouping_expr::Grouping_expr(const Expr* expr_arg) :
        expr {move(expr_arg)}
    {}

    void Grouping_expr::accept(const Expr* owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_cast<const Grouping_expr*>(owner_this));
    }

    /*
        struct Literal_expr
    */

    Literal_expr::Literal_expr(Literal&& value_arg) :
        value {move(value_arg)}
    {}

    void Literal_expr::accept(const Expr* owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_cast<const Literal_expr*>(owner_this));
    }

    /*
        struct Unary_expr
    */

    Unary_expr::Unary_expr(Token&& op_arg, const Expr* right_arg) :
        op {move(op_arg)},
        right {move(right_arg)}
    {}

    void Unary_expr::accept(const Expr* owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_cast<const Unary_expr*>(owner_this));
    }

    /*
        struct Var_expr
    */

    Var_expr::Var_expr(Token&& name_arg) :
        name {move(name_arg)}
    {}

    void Var_expr::accept(const Expr* owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_cast<const Var_expr*>(owner_this));
    }

    const Expr* Var_expr::make_assignment_expression(
        const Expr* lhs_expr,
        const Expr* rhs_expr,
        const Runtime_error& /*throwable_if_not_lvalue*/
    ) const {
        return new (GC_MALLOC(sizeof(Assign_expr))) Assign_expr{Token{static_cast<const Var_expr*>(lhs_expr)->name}, move(rhs_expr)};
    }

    /*
        struct Assign_expr
    */

    Assign_expr::Assign_expr(Token&& name_arg, const Expr* value_arg) :
        name {move(name_arg)},
        value {move(value_arg)}
    {}

    void Assign_expr::accept(const Expr* owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_cast<const Assign_expr*>(owner_this));
    }

    /*
        struct Logical_expr
    */

    Logical_expr::Logical_expr(const Expr* left_arg, Token&& op_arg, const Expr* right_arg) :
        left {move(left_arg)},
        op {move(op_arg)},
        right {move(right_arg)}
    {}

    void Logical_expr::accept(const Expr* owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_cast<const Logical_expr*>(owner_this));
    }

    /*
        struct Call_expr
    */

    Call_expr::Call_expr(
        const Expr* callee_arg,
        Token&& closing_paren_arg,
        vector<const Expr*>&& arguments_arg
    ) :
        callee {move(callee_arg)},
        closing_paren {move(closing_paren_arg)},
        arguments {move(arguments_arg)}
    {}

    void Call_expr::accept(const Expr* owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_cast<const Call_expr*>(owner_this));
    }

    /*
        struct Get_expr
    */

    Get_expr::Get_expr(const Expr* object_arg, Token&& name_arg) :
        object {move(object_arg)},
        name {move(name_arg)}
    {}

    void Get_expr::accept(const Expr* owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_cast<const Get_expr*>(owner_this));
    }

    const Expr* Get_expr::make_assignment_expression(
        const Expr* lhs_expr,
        const Expr* rhs_expr,
        const Runtime_error& /*throwable_if_not_lvalue*/
    ) const {
        return new (GC_MALLOC(sizeof(Set_expr))) Set_expr{
            static_cast<const Get_expr*>(lhs_expr)->object,
            Token{static_cast<const Get_expr*>(lhs_expr)->name},
            move(rhs_expr)
        };
    }

    /*
        struct Set_expr
    */

    Set_expr::Set_expr(const Expr* object_arg, Token&& name_arg, const Expr* value_arg) :
        object {move(object_arg)},
        name {move(name_arg)},
        value {move(value_arg)}
    {}

    void Set_expr::accept(const Expr* owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_cast<const Set_expr*>(owner_this));
    }

    /*
        struct This_expr
    */

    This_expr::This_expr(Token&& keyword_arg) :
        keyword {move(keyword_arg)}
    {}

    void This_expr::accept(const Expr* owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_cast<const This_expr*>(owner_this));
    }

    /*
        struct Super_expr
    */

    Super_expr::Super_expr(Token&& keyword_arg, Token&& method_arg) :
        keyword {move(keyword_arg)},
        method {move(method_arg)}
    {}

    void Super_expr::accept(const Expr* owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_cast<const Super_expr*>(owner_this));
    }
}}
