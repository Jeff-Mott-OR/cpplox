#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "expression_impls.hpp"
#include "expression_visitor.hpp"
#include "statement_impls.hpp"
#include "statement_visitor.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    enum class Var_binding { declared, defined };
    enum class Function_type { none, function, initializer, method };
    enum class Class_type { none, class_, subclass };

    class Resolver : public Expr_visitor, public Stmt_visitor {
        public:
            explicit Resolver(std::function<void (const Expr*, int depth)>&& on_resolve_local);

            void visit(const Block_stmt*) override;
            void visit(const Class_stmt*) override;
            void visit(const Var_stmt*) override;
            void visit(const Var_expr*) override;
            void visit(const Assign_expr*) override;
            void visit(const Function_stmt*) override;
            void visit(const Expr_stmt*) override;
            void visit(const If_stmt*) override;
            void visit(const Print_stmt*) override;
            void visit(const Return_stmt*) override;
            void visit(const While_stmt*) override;
            void visit(const Binary_expr*) override;
            void visit(const Call_expr*) override;
            void visit(const Get_expr*) override;
            void visit(const Set_expr*) override;
            void visit(const Super_expr*) override;
            void visit(const This_expr*) override;
            void visit(const Function_expr*) override;
            void visit(const Grouping_expr*) override;
            void visit(const Literal_expr*) override;
            void visit(const Logical_expr*) override;
            void visit(const Unary_expr*) override;

        private:
            std::vector<std::vector<std::pair<std::string, Var_binding>>> scopes_;
            Function_type current_function_type_ {Function_type::none};
            Class_type current_class_type_ {Class_type::none};
            std::function<void (const Expr*, int depth)> on_resolve_local_;

            std::pair<std::string, Var_binding>& declare_var(const Token& name);
            void resolve_function(const Function_expr*, Function_type);
            void resolve_local(const Expr&, const std::string& name);
    };

    struct Resolver_error : Runtime_error {
        using Runtime_error::Runtime_error;

        explicit Resolver_error(const std::string& what, const Token&);
    };
}}
