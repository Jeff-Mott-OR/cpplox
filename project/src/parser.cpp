#include "parser.hpp"

#include <utility>

#include "expression.hpp"

using std::make_unique;
using std::move;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;

// Allow the internal linkage section to access names
using namespace motts::lox;

// Not exported (internal linkage)
namespace {
    auto advance_if_match(Token_iterator& token_iter, const Token_type& token_type) {
        if (token_iter->type == token_type) {
            ++token_iter;
            return true;
        }

        return false;
    }

    auto consume(Token_iterator& token_iter, const Token_type& token_type, const string& error_msg) {
        if (token_iter->type != token_type) {
            throw Parser_error{error_msg, *token_iter};
        }
        const auto token = *move(token_iter);
        ++token_iter;

        return token;
    }

    unique_ptr<Expr> consume_expression(Token_iterator& token_iter);
    unique_ptr<Stmt> consume_declaration(Token_iterator& token_iter);

    unique_ptr<Expr> consume_primary(Token_iterator& token_iter) {
        if (advance_if_match(token_iter, Token_type::false_)) {
            return make_unique<Literal_expr>(Literal_multi_type{false});
        }
        if (advance_if_match(token_iter, Token_type::true_)) {
            return make_unique<Literal_expr>(Literal_multi_type{true});
        }
        if (advance_if_match(token_iter, Token_type::nil_)) {
            return make_unique<Literal_expr>(Literal_multi_type{nullptr});
        }

        if (token_iter->type == Token_type::number || token_iter->type == Token_type::string) {
            auto expr = make_unique<Literal_expr>(*((*move(token_iter)).literal));
            ++token_iter;
            return expr;
        }

        if (token_iter->type == Token_type::identifier) {
            auto expr = make_unique<Var_expr>(*move(token_iter));
            ++token_iter;
            return expr;
        }

        if (advance_if_match(token_iter, Token_type::left_paren)) {
            auto expr = consume_expression(token_iter);
            consume(token_iter, Token_type::right_paren, "Expected ')' after expression.");

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

    unique_ptr<Expr> consume_assignment(Token_iterator& token_iter) {
        auto lhs_expr = consume_equality(token_iter);

        if (token_iter->type == Token_type::equal) {
            const auto equals_token = *move(token_iter);
            ++token_iter;

            auto rhs_expr = consume_assignment(token_iter);

            // The lhs expression *could* turn out to be an l-value such as an identifier, but it could also turn out to be an r-value such
            // as a literal. Either one is exposed only as a base Expr interface, so we have to downcast and throw if it isn't an l-value.
            // But if you've read my other comments and past commit messages, you know I consider casts a smell. TODO Find an alternate way
            // to detect or remember l-value vs r-value-ness.
            auto var_expr = dynamic_cast<Var_expr*>(lhs_expr.get());
            if (!var_expr) {
                throw Parser_error{"Invalid assignment target.", equals_token};
            }

            return make_unique<Assign_expr>(move(var_expr->name), move(rhs_expr));
        }

        return lhs_expr;
    }

    unique_ptr<Expr> consume_expression(Token_iterator& token_iter) {
        return consume_assignment(token_iter);
    }

    unique_ptr<Stmt> consume_print_statement(Token_iterator& token_iter) {
        auto value = consume_expression(token_iter);
        consume(token_iter, Token_type::semicolon, "Expected ';' after value.");

        return make_unique<Print_stmt>(move(value));
    }

    unique_ptr<Stmt> consume_expression_statement(Token_iterator& token_iter) {
        auto expr = consume_expression(token_iter);
        consume(token_iter, Token_type::semicolon, "Expected ';' after expression.");

        return make_unique<Expr_stmt>(move(expr));
    }

    auto consume_block_statement(Token_iterator& token_iter) {
        vector<unique_ptr<Stmt>> statements;
        while (token_iter->type != Token_type::right_brace && token_iter->type != Token_type::eof) {
            statements.push_back(consume_declaration(token_iter));
        }
        consume(token_iter, Token_type::right_brace, "Expected '}' after block.");

        return statements;
    }

    unique_ptr<Stmt> consume_statement(Token_iterator& token_iter) {
        if (advance_if_match(token_iter, Token_type::print_)) {
            return consume_print_statement(token_iter);
        }
        if (advance_if_match(token_iter, Token_type::left_brace)) {
            return make_unique<Block_stmt>(consume_block_statement(token_iter));
        }

        return consume_expression_statement(token_iter);
    }

    unique_ptr<Stmt> consume_var_declaration(Token_iterator& token_iter) {
        auto var_name = consume(token_iter, Token_type::identifier, "Expected variable name.");

        unique_ptr<Expr> initializer;
        if (advance_if_match(token_iter, Token_type::equal)) {
            initializer = consume_expression(token_iter);
        }

        consume(token_iter, Token_type::semicolon, "Expected ';' after variable declaration.");

        return make_unique<Var_stmt>(move(var_name), move(initializer));
    }

    unique_ptr<Stmt> consume_declaration(Token_iterator& token_iter) {
        if (advance_if_match(token_iter, Token_type::var_)) {
            return consume_var_declaration(token_iter);
        }

        return consume_statement(token_iter);
    }

    void recover_to_synchronization_point(Token_iterator& token_iter) {
        while (token_iter->type != Token_type::eof) {
            // After a semicolon, we're probably finished with a statement. Use it as a synchronization point
            if (advance_if_match(token_iter, Token_type::semicolon)) {
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
    vector<unique_ptr<Stmt>> parse(Token_iterator&& token_iter) {
        vector<unique_ptr<Stmt>> statements;
        while (token_iter->type != Token_type::eof) {
            try {
                statements.push_back(consume_declaration(token_iter));
            } catch (const Parser_error&) {
                // TODO Log the error somewhere
                recover_to_synchronization_point(token_iter);
            }
        }

        return statements;
    }

    Parser_error::Parser_error(const string& what, const Token& token)
        : Runtime_error {
            "[Line " + to_string(token.line) + "] Error at " + (
                token.type != Token_type::eof ? "'" + token.lexeme + "'" : "end"
            ) + ": " + what
        }
    {}
}}
