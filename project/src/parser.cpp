#include "parser.hpp"

#include <string>
#include <utility>

using std::make_unique;
using std::move;
using std::runtime_error;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;

namespace motts { namespace lox {
    Parser::Parser(vector<Token>&& tokens_)
        : tokens {move(tokens_)},
          token_iter {tokens.begin()}
    {}

    std::unique_ptr<Expr> Parser::parse() {
        return consume_expression();
    }

    bool Parser::advance_if_any_match(Token_type expected) {
        if (token_iter->type != Token_type::eof && token_iter->type == expected) {
            ++token_iter;

            return true;
        }

        return false;
    }

    template<typename Token_type, typename... Token_types>
        bool Parser::advance_if_any_match(Token_type token_type, Token_types... token_types) {
            return advance_if_any_match(token_type) || advance_if_any_match(token_types...);
        }

    unique_ptr<Expr> Parser::consume_expression() {
        return consume_equality();
    }

    unique_ptr<Expr> Parser::consume_equality() {
        auto left_expr = consume_comparison();

        while(advance_if_any_match(Token_type::bang_equal, Token_type::equal_equal)) {
            auto operator_token = move(*(token_iter - 1));
            auto right_expr = consume_comparison();
            left_expr = make_unique<Binary_expr>(move(left_expr), move(operator_token), move(right_expr));
        }

        return left_expr;
    }

    unique_ptr<Expr> Parser::consume_comparison() {
        auto left_expr = consume_addition();

        while (advance_if_any_match(
            Token_type::greater, Token_type::greater_equal,
            Token_type::less, Token_type::less_equal
        )) {
            auto operator_token = move(*(token_iter - 1));
            auto right_expr = consume_addition();
            left_expr = make_unique<Binary_expr>(move(left_expr), move(operator_token), move(right_expr));
        }

        return left_expr;
    }

    unique_ptr<Expr> Parser::consume_addition() {
        auto left_expr = consume_multiplication();

        while (advance_if_any_match(Token_type::minus, Token_type::plus)) {
            auto operator_token = move(*(token_iter - 1));
            auto right_expr = consume_multiplication();
            left_expr = make_unique<Binary_expr>(move(left_expr), move(operator_token), move(right_expr));
        }

        return left_expr;
    }

    unique_ptr<Expr> Parser::consume_multiplication() {
        auto left_expr = consume_unary();

        while (advance_if_any_match(Token_type::slash, Token_type::star)) {
            auto operator_token = move(*(token_iter - 1));
            auto right_expr = consume_unary();
            left_expr = make_unique<Binary_expr>(move(left_expr), move(operator_token), move(right_expr));
        }

        return left_expr;
    }

    unique_ptr<Expr> Parser::consume_unary() {
        if (advance_if_any_match(Token_type::bang, Token_type::minus)) {
            auto operator_token = move(*(token_iter - 1));
            auto right_expr = consume_unary();
            return make_unique<Unary_expr>(move(operator_token), move(right_expr));
        }

        return consume_primary();
    }

    unique_ptr<Expr> Parser::consume_primary() {
        if (advance_if_any_match(Token_type::t_false)) {
            return make_unique<Literal_expr>(Literal_multi_type{false});
        }
        if (advance_if_any_match(Token_type::t_true)) {
            return make_unique<Literal_expr>(Literal_multi_type{true});
        }
        if (advance_if_any_match(Token_type::t_nil)) {
            return make_unique<Literal_expr>(Literal_multi_type{nullptr});
        }

        if (advance_if_any_match(Token_type::number, Token_type::string)) {
            return make_unique<Literal_expr>(move(*((token_iter - 1)->literal)));
        }

        if (advance_if_any_match(Token_type::left_paren)) {
            auto expr = consume_expression();
            if (!advance_if_any_match(Token_type::right_paren)) {
                throw Parser_error{"Expect ')' after expression.", *token_iter};
            }
            return make_unique<Grouping_expr>(move(expr));
        }

        throw Parser_error{"Expect expression.", *token_iter};
    }

    void Parser::recover_to_synchronization_point() {
        // Consume the offending token
        ++token_iter;

        while (token_iter->type != Token_type::eof) {
            if ((token_iter - 1)->type == Token_type::semicolon) {
                return;
            }

            // Warning! This switch doesn't handle every enumeration value and will emit a warning
            switch (token_iter->type) {
                case Token_type::t_class:
                case Token_type::t_fun:
                case Token_type::t_var:
                case Token_type::t_for:
                case Token_type::t_if:
                case Token_type::t_while:
                case Token_type::t_print:
                case Token_type::t_return:
                    return;
            }

            ++token_iter;
        }
    }

    Parser_error::Parser_error(const std::string& what, const Token& token)
      : runtime_error{
            "[Line " + to_string(token.line) + "] Error at " + (
                token.type != Token_type::eof ?
                    "'" + token.lexeme + "'" :
                    "end"
            ) + ": " + what
        }
    {}
}}
