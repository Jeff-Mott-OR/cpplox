#include "statement_impls.hpp"
#include <utility>
#include "statement_visitor.hpp"

using std::move;
using std::vector;

using gcpp::deferred_ptr;
using gcpp::static_pointer_cast;

namespace motts { namespace lox {
    /*
        struct Expr_stmt
    */

    Expr_stmt::Expr_stmt(deferred_ptr<const Expr>&& expr_arg) :
        expr {move(expr_arg)}
    {}

    void Expr_stmt::accept(const deferred_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Expr_stmt>(owner_this));
    }

    /*
        struct Print_stmt
    */

    Print_stmt::Print_stmt(deferred_ptr<const Expr>&& expr_arg) :
        expr {move(expr_arg)}
    {}

    void Print_stmt::accept(const deferred_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Print_stmt>(owner_this));
    }

    /*
        struct Var_stmt
    */

    Var_stmt::Var_stmt(Token&& name_arg, deferred_ptr<const Expr>&& initializer_arg) :
        name {move(name_arg)},
        initializer {move(initializer_arg)}
    {}

    void Var_stmt::accept(const deferred_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Var_stmt>(owner_this));
    }

    /*
        struct While_stmt
    */

    While_stmt::While_stmt(deferred_ptr<const Expr>&& condition_arg, deferred_ptr<const Stmt>&& body_arg) :
        condition {move(condition_arg)},
        body {move(body_arg)}
    {}

    void While_stmt::accept(const deferred_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const While_stmt>(owner_this));
    }

    /*
        struct For_stmt
    */

    For_stmt::For_stmt(deferred_ptr<const Expr>&& condition_arg, deferred_ptr<const Expr>&& increment_arg, deferred_ptr<const Stmt>&& body_arg) :
        condition {move(condition_arg)},
        increment {move(increment_arg)},
        body {move(body_arg)}
    {}

    void For_stmt::accept(const deferred_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const For_stmt>(owner_this));
    }

    /*
        struct Block_stmt
    */

    Block_stmt::Block_stmt(vector<deferred_ptr<const Stmt>>&& statements_arg) :
        statements {move(statements_arg)}
    {}

    void Block_stmt::accept(const deferred_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Block_stmt>(owner_this));
    }

    /*
        struct If_stmt
    */

    If_stmt::If_stmt(
        deferred_ptr<const Expr>&& condition_arg,
        deferred_ptr<const Stmt>&& then_branch_arg,
        deferred_ptr<const Stmt>&& else_branch_arg
    ) :
        condition {move(condition_arg)},
        then_branch {move(then_branch_arg)},
        else_branch {move(else_branch_arg)}
    {}

    void If_stmt::accept(const deferred_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const If_stmt>(owner_this));
    }

    /*
        struct Function_stmt
    */

    Function_stmt::Function_stmt(deferred_ptr<const Function_expr>&& expr_arg) :
        expr {move(expr_arg)}
    {}

    void Function_stmt::accept(const deferred_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Function_stmt>(owner_this));
    }

    /*
        struct Return_stmt
    */

    Return_stmt::Return_stmt(Token&& keyword_arg, deferred_ptr<const Expr>&& value_arg) :
        keyword {move(keyword_arg)},
        value {move(value_arg)}
    {}

    void Return_stmt::accept(const deferred_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Return_stmt>(owner_this));
    }

    /*
        struct Class_stmt
    */

    Class_stmt::Class_stmt(
        Token&& name_arg,
        deferred_ptr<const Var_expr>&& superclass_arg,
        vector<deferred_ptr<const Function_stmt>>&& methods_arg
    ) :
        name {move(name_arg)},
        superclass {move(superclass_arg)},
        methods {move(methods_arg)}
    {}

    void Class_stmt::accept(const deferred_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Class_stmt>(owner_this));
    }

    /*
        struct Break_stmt
    */

    Break_stmt::Break_stmt() = default;

    void Break_stmt::accept(const deferred_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Break_stmt>(owner_this));
    }

    /*
        struct Continue_stmt
    */

    Continue_stmt::Continue_stmt() = default;

    void Continue_stmt::accept(const deferred_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Continue_stmt>(owner_this));
    }
}}
