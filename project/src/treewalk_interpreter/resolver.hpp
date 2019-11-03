#pragma once

#include "resolver_fwd.hpp"

#include <string>
#include <unordered_map>
#include <vector>

#include "exception.hpp"
#include "expression_impls.hpp"
#include "expression_visitor.hpp"
#include "interpreter.hpp"
#include "statement_impls.hpp"
#include "statement_visitor.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    class Resolver : public Expr_visitor, public Stmt_visitor {
        public:
            explicit Resolver(Interpreter&);

            void visit(const gcpp::deferred_ptr<const Block_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Class_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Var_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Var_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Assign_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Function_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Expr_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const If_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Print_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Return_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const While_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const For_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Break_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Continue_stmt>&) override;
            void visit(const gcpp::deferred_ptr<const Binary_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Call_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Get_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Set_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Super_expr>&) override;
            void visit(const gcpp::deferred_ptr<const This_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Function_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Grouping_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Literal_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Logical_expr>&) override;
            void visit(const gcpp::deferred_ptr<const Unary_expr>&) override;

        private:
            enum class Var_binding { declared, defined };
            enum class Function_type { none, function, initializer, method };
            enum class Class_type { none, class_, subclass };

            using Scope = std::unordered_map<std::string, Var_binding>;
            std::vector<Scope> scopes_;

            Function_type current_function_type_ {Function_type::none};
            Class_type current_class_type_ {Class_type::none};
            Interpreter& interpreter_;

            Var_binding& declare_var(const Token& name);
            void resolve_function(const gcpp::deferred_ptr<const Function_expr>&, Function_type);
            void resolve_local(const gcpp::deferred_ptr<const Expr>&, const std::string& name);
    };

    struct Resolver_error : Runtime_error {
        using Runtime_error::Runtime_error;

        explicit Resolver_error(const std::string& what, const Token&);
    };
}}
