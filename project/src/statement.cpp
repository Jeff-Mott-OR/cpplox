#include "statement.hpp"

#include <utility>

using std::move;
using std::unique_ptr;
using std::vector;

namespace motts { namespace lox {
    /*
        struct Stmt
    */

    Stmt::Stmt() = default;

    Stmt::~Stmt() = default;

    /*
        struct Expr_stmt
    */

    Expr_stmt::Expr_stmt(unique_ptr<Expr>&& expr_arg)
        : expr {move(expr_arg)}
    {}

    void Expr_stmt::accept(Stmt_visitor& visitor) const {
        visitor.visit(*this);
    }

    /*
        struct Print_stmt
    */

    Print_stmt::Print_stmt(unique_ptr<Expr>&& expr_arg)
        : expr {move(expr_arg)}
    {}

    void Print_stmt::accept(Stmt_visitor& visitor) const {
        visitor.visit(*this);
    }

    /*
        struct Var_stmt
    */

    Var_stmt::Var_stmt(Token&& name_arg, unique_ptr<Expr>&& initializer_arg)
        : name {move(name_arg)},
          initializer {move(initializer_arg)}
    {}

    void Var_stmt::accept(Stmt_visitor& visitor) const {
        visitor.visit(*this);
    }

    /*
        struct Block_stmt
    */

    Block_stmt::Block_stmt(vector<unique_ptr<Stmt>>&& statements_arg)
        : statements {move(statements_arg)}
    {}

    void Block_stmt::accept(Stmt_visitor& visitor) const {
        visitor.visit(*this);
    }

    /*
        struct Stmt_visitor
    */

    Stmt_visitor::Stmt_visitor() = default;

    Stmt_visitor::~Stmt_visitor() = default;
}}
