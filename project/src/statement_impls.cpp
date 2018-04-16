// Related header
#include "statement_impls.hpp"
// C standard headers
// C++ standard headers
#include <algorithm>
#include <iterator>
#include <utility>
// Third-party headers
// This project's headers
#include "statement_visitor.hpp"

using std::back_inserter;
using std::make_unique;
using std::move;
using std::transform;
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

    unique_ptr<Stmt> Expr_stmt::clone() const {
        return make_unique<Expr_stmt>(expr ? expr->clone() : nullptr);
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

    unique_ptr<Stmt> Print_stmt::clone() const {
        return make_unique<Print_stmt>(expr ? expr->clone() : nullptr);
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

    unique_ptr<Stmt> Var_stmt::clone() const {
        return make_unique<Var_stmt>(Token{name}, initializer ? initializer->clone() : nullptr);
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

    unique_ptr<Stmt> While_stmt::clone() const {
        return make_unique<While_stmt>(condition ? condition->clone() : nullptr, body ? body->clone() : nullptr);
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

    unique_ptr<Stmt> Block_stmt::clone() const {
        vector<unique_ptr<Stmt>> statements_copy;
        transform(statements.cbegin(), statements.cend(), back_inserter(statements_copy), [] (const auto& stmt) {
            return stmt->clone();
        });

        return make_unique<Block_stmt>(move(statements_copy));
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

    unique_ptr<Stmt> If_stmt::clone() const {
        return make_unique<If_stmt>(
            condition ? condition->clone() : nullptr,
            then_branch ? then_branch->clone() : nullptr,
            else_branch ? else_branch->clone() : nullptr
        );
    }

    /*
        struct Function_stmt
    */

    Function_stmt::Function_stmt(
        Token&& name_arg,
        vector<Token>&& parameters_arg,
        vector<unique_ptr<Stmt>>&& body_arg
    ) :
        name {move(name_arg)},
        parameters {move(parameters_arg)},
        body {move(body_arg)}
    {}

    Function_stmt::Function_stmt(const Function_stmt& rhs) :
        Stmt {rhs},
        name {rhs.name},
        parameters {rhs.parameters}
    {
        transform(rhs.body.cbegin(), rhs.body.cend(), back_inserter(body), [] (const auto& stmt) {
            return stmt->clone();
        });
    }

    void Function_stmt::accept(Stmt_visitor& visitor) const {
        visitor.visit(*this);
    }

    unique_ptr<Stmt> Function_stmt::clone() const {
        return make_unique<Function_stmt>(*this);
    }

    /*
        struct Return_stmt
    */

    Return_stmt::Return_stmt(unique_ptr<Expr>&& value_arg) :
        value {move(value_arg)}
    {}

    void Return_stmt::accept(Stmt_visitor& visitor) const {
        visitor.visit(*this);
    }

    unique_ptr<Stmt> Return_stmt::clone() const {
        return make_unique<Return_stmt>(value ? value->clone() : nullptr);
    }
}}
