#pragma once

#include <memory>
#include <stdexcept>
#include <vector>

#include "expression.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    class Parser {
        public:
            explicit Parser(std::vector<Token>&&);
            std::unique_ptr<Expr> parse();

        private:
            std::vector<Token> tokens;
            std::vector<Token>::iterator token_iter;

            bool advance_if_any_match(Token_type expected);
            template<typename Token_type, typename... Token_types>
                bool advance_if_any_match(Token_type, Token_types...);
            std::unique_ptr<Expr> consume_expression();
            std::unique_ptr<Expr> consume_equality();
            std::unique_ptr<Expr> consume_comparison();
            std::unique_ptr<Expr> consume_addition();
            std::unique_ptr<Expr> consume_multiplication();
            std::unique_ptr<Expr> consume_unary();
            std::unique_ptr<Expr> consume_primary();

            void recover_to_synchronization_point();
    };

    class Parser_error : public std::runtime_error {
        public:
            Parser_error(const std::string& what, const Token&);
    };
}}
