#pragma once

// Related header
#include "interpreter_fwd.hpp"
// C standard headers
// C++ standard headers
#include <memory>
#include <string>
#include <unordered_map>
// Third-party headers
// This project's headers
#include "callable.hpp"
#include "exception.hpp"
#include "expression_visitor.hpp"
#include "statement_visitor.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    class Environment : std::unordered_map<std::string, Literal> {
        using super_ = std::unordered_map<std::string, Literal>;

        public:
            // The constructor that takes an enclosing environment would otherwise look like a copy constructor, so
            // distinguish it with tag dispatch.
            struct Enclosing_tag {};

            Environment();
            Environment(Environment& enclosing, Enclosing_tag);

            super_::iterator find(const std::string& var_name);

            // Borrow from base as needed
            using super_::end;
            using super_::operator[];

        private:
            Environment* enclosing_ {};
    };

    class Function : public Callable {
        public:
            explicit Function(std::shared_ptr<Function_stmt> declaration, std::shared_ptr<Environment> enclosed);
            Literal call(Interpreter&, const std::vector<Literal>& arguments) override;
            int arity() const override;
            std::string to_string() const override;

        private:
            std::shared_ptr<Function_stmt> declaration_;
            std::shared_ptr<Environment> enclosed_;
    };

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

            std::shared_ptr<Environment> environment_ {std::make_shared<Environment>()};
            std::shared_ptr<Environment> globals_ {environment_};

            friend class Function;
    };

    struct Interpreter_error : Runtime_error {
        using Runtime_error::Runtime_error;

        explicit Interpreter_error(const std::string& what, const Token&);
    };

    struct Return {
        Literal value;
    };
}}
