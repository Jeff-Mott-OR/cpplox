#pragma once

// Related header
#include "interpreter_fwd.hpp"
// C standard headers
// C++ standard headers
#include <memory>
#include <string>
// Third-party headers
// This project's headers
#include "callable.hpp"
#include "exception.hpp"
#include "expression_visitor.hpp"
#include "statement_visitor.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    class Environment;

    class Interpreter : public Expr_visitor, public Stmt_visitor {
        public:
            explicit Interpreter();

            void visit(const Binary_expr&) override;
            void visit(const Grouping_expr&) override;
            void visit(const Literal_expr&) override;
            void visit(const Unary_expr&) override;
            void visit(const Var_expr&) override;
            void visit(const Assign_expr&) override;
            void visit(const Logical_expr&) override;
            void visit(const Call_expr&) override;

            void visit(const Expr_stmt&) override;
            void visit(const If_stmt&) override;
            void visit(const Print_stmt&) override;
            void visit(const While_stmt&) override;
            void visit(const Var_stmt&) override;
            void visit(const Block_stmt&) override;
            void visit(const Function_stmt&) override;
            void visit(const Return_stmt&) override;

            const Literal& result() const &;
            Literal&& result() &&;

        private:
            Literal result_;
            bool returning_ {false};

            std::shared_ptr<Environment> environment_;
            std::shared_ptr<Environment> globals_;

            class Function;
    };

    struct Interpreter_error : Runtime_error {
        using Runtime_error::Runtime_error;

        explicit Interpreter_error(const std::string& what, const Token&);
    };
}}
