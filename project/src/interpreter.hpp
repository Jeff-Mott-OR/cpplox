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

            void visit(const std::shared_ptr<const Binary_expr>&) override;
            void visit(const std::shared_ptr<const Grouping_expr>&) override;
            void visit(const std::shared_ptr<const Literal_expr>&) override;
            void visit(const std::shared_ptr<const Unary_expr>&) override;
            void visit(const std::shared_ptr<const Var_expr>&) override;
            void visit(const std::shared_ptr<const Assign_expr>&) override;
            void visit(const std::shared_ptr<const Logical_expr>&) override;
            void visit(const std::shared_ptr<const Call_expr>&) override;

            void visit(const std::shared_ptr<const Expr_stmt>&) override;
            void visit(const std::shared_ptr<const If_stmt>&) override;
            void visit(const std::shared_ptr<const Print_stmt>&) override;
            void visit(const std::shared_ptr<const While_stmt>&) override;
            void visit(const std::shared_ptr<const Var_stmt>&) override;
            void visit(const std::shared_ptr<const Block_stmt>&) override;
            void visit(const std::shared_ptr<const Function_stmt>&) override;
            void visit(const std::shared_ptr<const Return_stmt>&) override;

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
