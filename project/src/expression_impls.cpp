#include "expression_impls.hpp"
#include <utility>
#include "expression_visitor.hpp"

using std::move;
using std::vector;

using boost::optional;
using deferred_heap_t = gcpp::deferred_heap;
using gcpp::deferred_ptr;
using gcpp::static_pointer_cast;

namespace motts { namespace lox {
    /*
        struct Binary_expr
    */

    Binary_expr::Binary_expr(deferred_ptr<const Expr>&& left_arg, Token&& op_arg, deferred_ptr<const Expr>&& right_arg) :
        left {move(left_arg)},
        op {move(op_arg)},
        right {move(right_arg)}
    {}

    void Binary_expr::accept(const deferred_ptr<const Expr>& owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Binary_expr>(owner_this));
    }

    /*
        struct Grouping_expr
    */

    Grouping_expr::Grouping_expr(deferred_ptr<const Expr>&& expr_arg) :
        expr {move(expr_arg)}
    {}

    void Grouping_expr::accept(const deferred_ptr<const Expr>& owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Grouping_expr>(owner_this));
    }

    /*
        struct Literal_expr
    */

    Literal_expr::Literal_expr(Literal&& value_arg) :
        value {move(value_arg)}
    {}

    void Literal_expr::accept(const deferred_ptr<const Expr>& owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Literal_expr>(owner_this));
    }

    /*
        struct Unary_expr
    */

    Unary_expr::Unary_expr(Token&& op_arg, deferred_ptr<const Expr>&& right_arg) :
        op {move(op_arg)},
        right {move(right_arg)}
    {}

    void Unary_expr::accept(const deferred_ptr<const Expr>& owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Unary_expr>(owner_this));
    }

    /*
        struct Var_expr
    */

    Var_expr::Var_expr(Token&& name_arg) :
        name {move(name_arg)}
    {}

    void Var_expr::accept(const deferred_ptr<const Expr>& owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Var_expr>(owner_this));
    }

    deferred_ptr<const Expr> Var_expr::make_assignment_expression(
        deferred_ptr<const Expr>&& lhs_expr,
        deferred_ptr<const Expr>&& rhs_expr,
        deferred_heap_t& deferred_heap,
        const Runtime_error& /*throwable_if_not_lvalue*/
    ) const {
        return deferred_heap.make<Assign_expr>(Token{static_pointer_cast<const Var_expr>(lhs_expr)->name}, move(rhs_expr));
    }

    /*
        struct Assign_expr
    */

    Assign_expr::Assign_expr(Token&& name_arg, deferred_ptr<const Expr>&& value_arg) :
        name {move(name_arg)},
        value {move(value_arg)}
    {}

    void Assign_expr::accept(const deferred_ptr<const Expr>& owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Assign_expr>(owner_this));
    }

    /*
        struct Logical_expr
    */

    Logical_expr::Logical_expr(deferred_ptr<const Expr>&& left_arg, Token&& op_arg, deferred_ptr<const Expr>&& right_arg) :
        left {move(left_arg)},
        op {move(op_arg)},
        right {move(right_arg)}
    {}

    void Logical_expr::accept(const deferred_ptr<const Expr>& owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Logical_expr>(owner_this));
    }

    /*
        struct Call_expr
    */

    Call_expr::Call_expr(
        deferred_ptr<const Expr>&& callee_arg,
        Token&& closing_paren_arg,
        vector<deferred_ptr<const Expr>>&& arguments_arg
    ) :
        callee {move(callee_arg)},
        closing_paren {move(closing_paren_arg)},
        arguments {move(arguments_arg)}
    {}

    void Call_expr::accept(const deferred_ptr<const Expr>& owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Call_expr>(owner_this));
    }

    /*
        struct Get_expr
    */

    Get_expr::Get_expr(deferred_ptr<const Expr>&& object_arg, Token&& name_arg) :
        object {move(object_arg)},
        name {move(name_arg)}
    {}

    void Get_expr::accept(const deferred_ptr<const Expr>& owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Get_expr>(owner_this));
    }

    deferred_ptr<const Expr> Get_expr::make_assignment_expression(
        deferred_ptr<const Expr>&& lhs_expr,
        deferred_ptr<const Expr>&& rhs_expr,
        deferred_heap_t& deferred_heap,
        const Runtime_error& /*throwable_if_not_lvalue*/
    ) const {
        return deferred_heap.make<Set_expr>(
            deferred_ptr<const Expr>{static_pointer_cast<const Get_expr>(lhs_expr)->object},
            Token{static_pointer_cast<const Get_expr>(lhs_expr)->name},
            move(rhs_expr)
        );
    }

    /*
        struct Set_expr
    */

    Set_expr::Set_expr(deferred_ptr<const Expr>&& object_arg, Token&& name_arg, deferred_ptr<const Expr>&& value_arg) :
        object {move(object_arg)},
        name {move(name_arg)},
        value {move(value_arg)}
    {}

    void Set_expr::accept(const deferred_ptr<const Expr>& owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Set_expr>(owner_this));
    }

    /*
        struct This_expr
    */

    This_expr::This_expr(Token&& keyword_arg) :
        keyword {move(keyword_arg)}
    {}

    void This_expr::accept(const deferred_ptr<const Expr>& owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const This_expr>(owner_this));
    }

    /*
        struct Super_expr
    */

    Super_expr::Super_expr(Token&& keyword_arg, Token&& method_arg) :
        keyword {move(keyword_arg)},
        method {move(method_arg)}
    {}

    void Super_expr::accept(const deferred_ptr<const Expr>& owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Super_expr>(owner_this));
    }

    /*
        struct Function_expr
    */

    Function_expr::Function_expr(
        optional<Token>&& name_arg,
        vector<Token>&& parameters_arg,
        vector<deferred_ptr<const Stmt>>&& body_arg
    ) :
        name {std::move(name_arg)},
        parameters {move(parameters_arg)},
        body {move(body_arg)}
    {}

    void Function_expr::accept(const deferred_ptr<const Expr>& owner_this, Expr_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Function_expr>(owner_this));
    }
}}
