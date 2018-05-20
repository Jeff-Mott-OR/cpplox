#pragma once

// Related header
#include "interpreter_fwd.hpp"
// C standard headers
// C++ standard headers
#include <memory>
#include <string>
#include <utility>
#include <vector>
// Third-party headers
// This project's headers
#include "callable.hpp"
#include "class.hpp"
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
            void visit(const std::shared_ptr<const Get_expr>&) override;
            void visit(const std::shared_ptr<const Set_expr>&) override;
            void visit(const std::shared_ptr<const Super_expr>&) override;
            void visit(const std::shared_ptr<const This_expr>&) override;

            void visit(const std::shared_ptr<const Expr_stmt>&) override;
            void visit(const std::shared_ptr<const If_stmt>&) override;
            void visit(const std::shared_ptr<const Print_stmt>&) override;
            void visit(const std::shared_ptr<const While_stmt>&) override;
            void visit(const std::shared_ptr<const Var_stmt>&) override;
            void visit(const std::shared_ptr<const Block_stmt>&) override;
            void visit(const std::shared_ptr<const Class_stmt>&) override;
            void visit(const std::shared_ptr<const Function_stmt>&) override;
            void visit(const std::shared_ptr<const Return_stmt>&) override;

            const Literal& result() const &;
            Literal&& result() &&;

            void resolve(const Expr*, int depth);

            friend class Function;

        private:
            std::vector<std::pair<std::string, Literal>>::iterator lookup_variable(const std::string& name, const Expr&);

            Literal result_;
            bool returning_ {false};

            std::shared_ptr<Environment> environment_;
            std::shared_ptr<Environment> globals_;
            std::vector<std::pair<const Expr*, int>> scope_depths_;
    };

    class Function : public Callable {
        public:
            explicit Function(
                std::shared_ptr<const Function_stmt> declaration,
                std::shared_ptr<Environment> enclosed,
                bool is_initializer = false
            );
            Literal call(
                const std::shared_ptr<const Callable>& /*owner_this*/,
                Interpreter& interpreter,
                const std::vector<Literal>& arguments
            ) const override;
            int arity() const override;
            std::string to_string() const override;
            std::shared_ptr<Function> bind(const std::shared_ptr<Instance>&) const;

        private:
            std::shared_ptr<const Function_stmt> declaration_;
            std::shared_ptr<Environment> enclosed_;
            bool is_initializer_;
    };

    struct Interpreter_error : Runtime_error {
        using Runtime_error::Runtime_error;

        explicit Interpreter_error(const std::string& what, const Token&);
    };
}}
