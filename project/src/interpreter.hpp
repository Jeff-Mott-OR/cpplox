#pragma once

// Related header
#include "interpreter_fwd.hpp"
// C standard headers
// C++ standard headers
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

            void visit(const Binary_expr*) override;
            void visit(const Grouping_expr*) override;
            void visit(const Literal_expr*) override;
            void visit(const Unary_expr*) override;
            void visit(const Var_expr*) override;
            void visit(const Assign_expr*) override;
            void visit(const Logical_expr*) override;
            void visit(const Call_expr*) override;
            void visit(const Get_expr*) override;
            void visit(const Set_expr*) override;
            void visit(const Super_expr*) override;
            void visit(const This_expr*) override;

            void visit(const Expr_stmt*) override;
            void visit(const If_stmt*) override;
            void visit(const Print_stmt*) override;
            void visit(const While_stmt*) override;
            void visit(const Var_stmt*) override;
            void visit(const Block_stmt*) override;
            void visit(const Class_stmt*) override;
            void visit(const Function_stmt*) override;
            void visit(const Return_stmt*) override;

            const Literal& result() const &;
            Literal&& result() &&;

            void resolve(const Expr*, int depth);

            friend class Function;

        private:
            std::vector<std::pair<std::string, Literal>>::iterator lookup_variable(const std::string& name, const Expr&);

            Literal result_;
            bool returning_ {false};

            Environment* environment_ {};
            Environment* globals_ {};
            std::vector<std::pair<const Expr*, int>> scope_depths_;
    };

    class Function : public Callable {
        public:
            explicit Function(
                const Function_stmt* declaration,
                Environment* enclosed,
                bool is_initializer = false
            );
            Literal call(
                const Callable* /*owner_this*/,
                Interpreter& interpreter,
                const std::vector<Literal>& arguments
            ) const override;
            int arity() const override;
            std::string to_string() const override;
            Function* bind(Instance*) const;

        private:
            const Function_stmt* declaration_ {};
            Environment* enclosed_ {};
            bool is_initializer_;
    };

    struct Interpreter_error : Runtime_error {
        using Runtime_error::Runtime_error;

        explicit Interpreter_error(const std::string& what, const Token&);
    };
}}
