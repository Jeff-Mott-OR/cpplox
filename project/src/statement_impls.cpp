// Related header
#include "statement_impls.hpp"
// C standard headers
// C++ standard headers
#include <utility>
// Third-party headers
// This project's headers
#include "statement_visitor.hpp"

using std::move;
using std::unique_ptr;
using std::vector;

namespace motts { namespace lox {
    /*
        struct Expr_stmt
    */

    Expr_stmt::Expr_stmt(unique_ptr<Expr>&& expr_arg) :
        expr {move(expr_arg)}
    {}

    void Expr_stmt::accept(Stmt_visitor& visitor) const {
        visitor.visit(*this);
    }

    /*
        struct Print_stmt
    */

    Print_stmt::Print_stmt(unique_ptr<Expr>&& expr_arg) :
        expr {move(expr_arg)}
    {}

    void Print_stmt::accept(Stmt_visitor& visitor) const {
        visitor.visit(*this);
    }

    /*
        struct Var_stmt
    */

    Var_stmt::Var_stmt(Token&& name_arg, unique_ptr<Expr>&& initializer_arg) :
        name {move(name_arg)},
        initializer {move(initializer_arg)}
    {}

    void Var_stmt::accept(Stmt_visitor& visitor) const {
        visitor.visit(*this);
    }

    /*
        struct While_stmt
    */

    While_stmt::While_stmt(unique_ptr<Expr>&& condition_arg, unique_ptr<Stmt>&& body_arg) :
        condition {move(condition_arg)},
        body {move(body_arg)}
    {}

    void While_stmt::accept(Stmt_visitor& visitor) const {
        visitor.visit(*this);
    }

    /*
        struct Block_stmt
    */

    Block_stmt::Block_stmt(vector<unique_ptr<Stmt>>&& statements_arg) :
        statements {move(statements_arg)}
    {}

    void Block_stmt::accept(Stmt_visitor& visitor) const {
        visitor.visit(*this);
    }

    /*
        struct If_stmt
    */

    If_stmt::If_stmt(
        unique_ptr<Expr>&& condition_arg,
        unique_ptr<Stmt>&& then_branch_arg,
        unique_ptr<Stmt>&& else_branch_arg
    ) :
        condition {move(condition_arg)},
        then_branch {move(then_branch_arg)},
        else_branch {move(else_branch_arg)}
    {}

    void If_stmt::accept(Stmt_visitor& visitor) const {
        visitor.visit(*this);
    }
}}
