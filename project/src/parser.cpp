#include "parser.hpp"

#include <utility>

using std::make_unique;
using std::move;
using std::string;
using std::to_string;
using std::unique_ptr;

// Allow the internal linkage section to access names
using namespace motts::lox;

// Not exported (internal linkage)
namespace {
    unique_ptr<Expr> consume_expression(Token_iterator& token_iter);

    unique_ptr<Expr> consume_primary(Token_iterator& token_iter) {
        if (token_iter->type == Token_type::false_) {
            ++token_iter;
            return make_unique<Literal_expr>(Literal_multi_type{false});
        }
        if (token_iter->type == Token_type::true_) {
            ++token_iter;
            return make_unique<Literal_expr>(Literal_multi_type{true});
        }
        if (token_iter->type == Token_type::nil_) {
            ++token_iter;
            return make_unique<Literal_expr>(Literal_multi_type{nullptr});
        }

        if (token_iter->type == Token_type::number || token_iter->type == Token_type::string) {
            auto expr = make_unique<Literal_expr>(*((*move(token_iter)).literal));
            ++token_iter;
            return expr;
        }

        if (token_iter->type == Token_type::left_paren) {
            ++token_iter;

            auto expr = consume_expression(token_iter);
            if (token_iter->type != Token_type::right_paren) {
                throw Parser_error{"Expected ')' after expression.", *token_iter};
            }
            ++token_iter;

            return make_unique<Grouping_expr>(move(expr));
        }

        throw Parser_error{"Expected expression.", *token_iter};
    }

    unique_ptr<Expr> consume_unary(Token_iterator& token_iter) {
        if (token_iter->type == Token_type::bang || token_iter->type == Token_type::minus) {
            auto operator_token = *move(token_iter);
            ++token_iter;
            auto right_expr = consume_unary(token_iter);

            return make_unique<Unary_expr>(move(operator_token), move(right_expr));
        }

        return consume_primary(token_iter);
    }

    unique_ptr<Expr> consume_multiplication(Token_iterator& token_iter) {
        auto left_expr = consume_unary(token_iter);

        while (token_iter->type == Token_type::slash || token_iter->type == Token_type::star) {
            auto operator_token = *move(token_iter);
            ++token_iter;
            auto right_expr = consume_unary(token_iter);

            left_expr = make_unique<Binary_expr>(move(left_expr), move(operator_token), move(right_expr));
        }

        return left_expr;
    }

    unique_ptr<Expr> consume_addition(Token_iterator& token_iter) {
        auto left_expr = consume_multiplication(token_iter);

        while (token_iter->type == Token_type::minus || token_iter->type == Token_type::plus) {
            auto operator_token = *move(token_iter);
            ++token_iter;
            auto right_expr = consume_multiplication(token_iter);

            left_expr = make_unique<Binary_expr>(move(left_expr), move(operator_token), move(right_expr));
        }

        return left_expr;
    }

    unique_ptr<Expr> consume_comparison(Token_iterator& token_iter) {
        auto left_expr = consume_addition(token_iter);

        while (
            token_iter->type == Token_type::greater || token_iter->type == Token_type::greater_equal ||
            token_iter->type == Token_type::less || token_iter->type == Token_type::less_equal
        ) {
            auto operator_token = *move(token_iter);
            ++token_iter;
            auto right_expr = consume_addition(token_iter);

            left_expr = make_unique<Binary_expr>(move(left_expr), move(operator_token), move(right_expr));
        }

        return left_expr;
    }

    unique_ptr<Expr> consume_equality(Token_iterator& token_iter) {
        auto left_expr = consume_comparison(token_iter);

        while (token_iter->type == Token_type::bang_equal || token_iter->type == Token_type::equal_equal) {
            auto operator_token = *move(token_iter);
            ++token_iter;
            auto right_expr = consume_comparison(token_iter);

            left_expr = make_unique<Binary_expr>(move(left_expr), move(operator_token), move(right_expr));
        }

        return left_expr;
    }

    unique_ptr<Expr> consume_expression(Token_iterator& token_iter) {
        return consume_equality(token_iter);
    }

    void recover_to_synchronization_point(Token_iterator& token_iter) {
        while (token_iter->type != Token_type::eof) {
            // After a semicolon, we're probably finished with a statement. Use it as a synchronization point
            if (token_iter->type == Token_type::semicolon) {
                ++token_iter;
                return;
            }

            // Most statements start with a keyword -- for, if, return, var, etc. Use them as synchronization points.
            // Warning! This switch doesn't handle every enumeration value and will emit a warning.
            switch (token_iter->type) {
                case Token_type::class_:
                case Token_type::fun_:
                case Token_type::var_:
                case Token_type::for_:
                case Token_type::if_:
                case Token_type::while_:
                case Token_type::print_:
                case Token_type::return_:
                    return;
            }

            // Discard tokens until we find a statement boundary
            ++token_iter;
        }
    }
}

// Exported (external linkage)
namespace motts { namespace lox {
    unique_ptr<Expr> parse(Token_iterator&& begin) {
        return consume_expression(begin);
    }

    Parser_error::Parser_error(const string& what, const Token& token)
        : Runtime_error {
            "[Line " + to_string(token.line) + "] Error at " + (
                token.type != Token_type::eof ? "'" + token.lexeme + "'" : "end"
            ) + ": " + what
        }
    {}
}}
