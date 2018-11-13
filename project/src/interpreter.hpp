#pragma once

#include "interpreter_fwd.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gc.h>
#include <gsl/gsl_util>

#include "environment.hpp"
#include "exception.hpp"
#include "expression.hpp"
#include "expression_visitor.hpp"
#include "literal.hpp"
#include "resolver.hpp"
#include "statement_visitor.hpp"
#include "token.hpp"

namespace motts { namespace lox {
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
            void visit(const Function_expr*) override;

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

            std::unique_ptr<Resolver> make_resolver();

        /*package_private:*/
            bool _returning() const;
            Literal&& _return_value() &&;
            gsl::final_action<std::function<void ()>> _push_environment(Environment*);

        private:
            Literal result_;
            bool returning_ {false};

            Environment* environment_ {new (GC_MALLOC(sizeof(Environment))) Environment{}};
            Environment* globals_ {environment_};
            std::vector<std::pair<const Expr*, int>> scope_depths_;

            std::vector<std::pair<std::string, Literal>>::iterator lookup_variable(const std::string& name, const Expr&);
    };

    struct Interpreter_error : Runtime_error {
        using Runtime_error::Runtime_error;

        explicit Interpreter_error(const std::string& what, const Token&);
    };
}}
