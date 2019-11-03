#pragma once

#include <string>
#include <unordered_map>

#include <gsl/gsl_util>
#pragma warning(push, 0)
    #include <deferred_heap.h>
#pragma warning(pop)

#include "environment.hpp"
#include "exception.hpp"
#include "expression.hpp"
#include "expression_visitor.hpp"
#include "function_fwd.hpp"
#include "literal.hpp"
#include "resolver_fwd.hpp"
#include "statement_visitor.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    class Interpreter : public Expr_visitor, public Stmt_visitor {
        public:
            explicit Interpreter(gcpp::deferred_heap&);

            void visit(const gcpp::deferred_ptr<const Binary_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Grouping_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Literal_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Unary_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Var_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Assign_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Logical_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Call_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Get_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Set_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Super_expr>&) override;
            void visit(const gcpp::deferred_ptr<const This_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Function_expr>&) override;

            void visit(const gcpp::deferred_ptr<const Expr_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const If_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Print_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const While_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const For_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Break_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Continue_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Var_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Block_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Class_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Function_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Return_stmt>&) override;

            const Literal& result() const &;
            Literal&& result() &&;

        private:
            gcpp::deferred_heap& deferred_heap_;
            gcpp::deferred_ptr<Environment> environment_ {deferred_heap_.make<Environment>()};
            gcpp::deferred_ptr<Environment> globals_ {environment_};

            Literal result_;
            bool returning_ {false};

            std::unordered_map<const Expr*, int> scope_depths_;
            Environment::iterator lookup_variable(const std::string& name, const Expr&);

            // Even though Resolver has access to everything, it's only intended to call the functions listed here
            friend Resolver;
            void resolve(const Expr*, int depth);

            // Even though Function has access to everything, it's only intended to call the functions listed here
            friend Function;
            void execute_block(const std::vector<gcpp::deferred_ptr<const Stmt>>& statements, const gcpp::deferred_ptr<Environment>&);
            bool returning() const;
            void returning(bool);
    };

    struct Interpreter_error : Runtime_error {
        using Runtime_error::Runtime_error;

        explicit Interpreter_error(const std::string& what, const Token&);
    };
}}
