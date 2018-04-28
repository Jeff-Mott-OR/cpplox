// Related header
#include "statement_impls.hpp"
// C standard headers
// C++ standard headers
#include <utility>
// Third-party headers
// This project's headers
#include "statement_visitor.hpp"

using std::move;
using std::shared_ptr;
using std::static_pointer_cast;
using std::vector;

namespace motts { namespace lox {
    /*
        struct Expr_stmt
    */

    Expr_stmt::Expr_stmt(shared_ptr<const Expr>&& expr_arg) :
        expr {move(expr_arg)}
    {}

    void Expr_stmt::accept(const shared_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Expr_stmt>(owner_this));
    }

    /*
        struct Print_stmt
    */

    Print_stmt::Print_stmt(shared_ptr<const Expr>&& expr_arg) :
        expr {move(expr_arg)}
    {}

    void Print_stmt::accept(const shared_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Print_stmt>(owner_this));
    }

    /*
        struct Var_stmt
    */

    Var_stmt::Var_stmt(Token&& name_arg, shared_ptr<const Expr>&& initializer_arg) :
        name {move(name_arg)},
        initializer {move(initializer_arg)}
    {}

    void Var_stmt::accept(const shared_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Var_stmt>(owner_this));
    }

    /*
        struct While_stmt
    */

    While_stmt::While_stmt(shared_ptr<const Expr>&& condition_arg, shared_ptr<const Stmt>&& body_arg) :
        condition {move(condition_arg)},
        body {move(body_arg)}
    {}

    void While_stmt::accept(const shared_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const While_stmt>(owner_this));
    }

    /*
        struct Block_stmt
    */

    Block_stmt::Block_stmt(vector<shared_ptr<const Stmt>>&& statements_arg) :
        statements {move(statements_arg)}
    {}

    void Block_stmt::accept(const shared_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Block_stmt>(owner_this));
    }

    /*
        struct If_stmt
    */

    If_stmt::If_stmt(
        shared_ptr<const Expr>&& condition_arg,
        shared_ptr<const Stmt>&& then_branch_arg,
        shared_ptr<const Stmt>&& else_branch_arg
    ) :
        condition {move(condition_arg)},
        then_branch {move(then_branch_arg)},
        else_branch {move(else_branch_arg)}
    {}

    void If_stmt::accept(const shared_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const If_stmt>(owner_this));
    }

    /*
        struct Function_stmt
    */

    Function_stmt::Function_stmt(
        Token&& name_arg,
        vector<Token>&& parameters_arg,
        vector<shared_ptr<const Stmt>>&& body_arg
    ) :
        name {move(name_arg)},
        parameters {move(parameters_arg)},
        body {move(body_arg)}
    {}

    void Function_stmt::accept(const shared_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Function_stmt>(owner_this));
    }

    /*
        struct Return_stmt
    */

    Return_stmt::Return_stmt(Token&& keyword_arg, shared_ptr<const Expr>&& value_arg) :
        keyword {move(keyword_arg)},
        value {move(value_arg)}
    {}

    void Return_stmt::accept(const shared_ptr<const Stmt>& owner_this, Stmt_visitor& visitor) const {
        visitor.visit(static_pointer_cast<const Return_stmt>(owner_this));
    }
}}
