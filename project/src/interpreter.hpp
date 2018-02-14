#pragma once

#include <stdexcept>

#include "expression.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    class Interpreter : public Expr_visitor {
        public:
            void visit(const Binary_expr&) override;
            void visit(const Grouping_expr&) override;
            void visit(const Literal_expr&) override;
            void visit(const Unary_expr&) override;

            const Literal_multi_type& result() const;

        private:
            Literal_multi_type result_;
    };

    class Interpreter_error : public std::runtime_error {
        public:
            using std::runtime_error::runtime_error;

            Interpreter_error(const std::string& what, const Token&);
    };
}}
