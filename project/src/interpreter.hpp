#pragma once

// Related header
// C standard headers
// C++ standard headers
#include <memory>
#include <string>
#include <unordered_map>
// Third-party headers
// This project's headers
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

    class Interpreter : public Expr_visitor, public Stmt_visitor {
        public:
            void visit(const Binary_expr&) override;
            void visit(const Grouping_expr&) override;
            void visit(const Literal_expr&) override;
            void visit(const Unary_expr&) override;
            void visit(const Var_expr&) override;
            void visit(const Assign_expr&) override;
            void visit(const Logical_expr&) override;

            void visit(const Expr_stmt&) override;
            void visit(const If_stmt&) override;
            void visit(const Print_stmt&) override;
            void visit(const While_stmt&) override;
            void visit(const Var_stmt&) override;
            void visit(const Block_stmt&) override;

            const Literal& result() const &;
            Literal&& result() &&;

        private:
            Literal result_;
            std::unique_ptr<Environment> environment_ {new Environment{}};
    };

    struct Interpreter_error : Runtime_error {
        using Runtime_error::Runtime_error;

        explicit Interpreter_error(const std::string& what, const Token&);
    };
}}
