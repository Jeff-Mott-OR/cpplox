// Related header
#include "parser.hpp"
// C standard headers
// C++ standard headers
#include <utility>
// Third-party headers
#include <boost/algorithm/string/join.hpp>
// This project's headers
#include "expression_impls.hpp"
#include "statement_impls.hpp"

using std::make_unique;
using std::move;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;

using boost::algorithm::join;

// Allow the internal linkage section to access names
using namespace motts::lox;

// Not exported (internal linkage)
namespace {
    class Parser {
        Token_iterator token_iter_;

        public:
            Parser(Token_iterator&& token_iter) :
                token_iter_ {move(token_iter)}
            {}

            const Token_iterator& token_iter() {
                return token_iter_;
            }

            unique_ptr<Stmt> consume_declaration() {
                if (advance_if_match(Token_type::fun_)) return consume_function_declaration();
                if (advance_if_match(Token_type::var_)) return consume_var_declaration();

                return consume_statement();
            }

            unique_ptr<Stmt> consume_var_declaration() {
                auto var_name = consume(Token_type::identifier, "Expected variable name.");

                unique_ptr<Expr> initializer;
                if (advance_if_match(Token_type::equal)) {
                    initializer = consume_expression();
                }

                consume(Token_type::semicolon, "Expected ';' after variable declaration.");

                return make_unique<Var_stmt>(move(var_name), move(initializer));
            }

            unique_ptr<Stmt> consume_function_declaration() {
                auto name = consume(Token_type::identifier, "Expected function name.");

                consume(Token_type::left_paren, "Expected '(' after function name.");
                vector<Token> parameters;
                if (token_iter_->type != Token_type::right_paren) {
                    do {
                        parameters.push_back(consume(Token_type::identifier, "Expected parameter name."));
                    } while (advance_if_match(Token_type::comma));

                    if (parameters.size() > 8) {
                        throw Parser_error{"Cannot have more than 8 parameters.", *token_iter_};
                    }
                }
                consume(Token_type::right_paren, "Expected ')' after parameters.");

                consume(Token_type::left_brace, "Expected '{' before function body.");
                auto body = consume_block_statement();

                return make_unique<Function_stmt>(move(name), move(parameters), move(body));
            }

            unique_ptr<Stmt> consume_statement() {
                if (advance_if_match(Token_type::for_)) return consume_for_statement();
                if (advance_if_match(Token_type::if_)) return consume_if_statement();
                if (advance_if_match(Token_type::print_)) return consume_print_statement();
                if (advance_if_match(Token_type::return_)) return consume_return_statement();
                if (advance_if_match(Token_type::while_)) return consume_while_statement();
                if (advance_if_match(Token_type::left_brace)) return make_unique<Block_stmt>(consume_block_statement());

                return consume_expression_statement();
            }

            unique_ptr<Stmt> consume_expression_statement() {
                auto expr = consume_expression();
                consume(Token_type::semicolon, "Expected ';' after expression.");

                return make_unique<Expr_stmt>(move(expr));
            }

            vector<unique_ptr<Stmt>> consume_block_statement() {
                vector<unique_ptr<Stmt>> statements;
                while (token_iter_->type != Token_type::right_brace && token_iter_->type != Token_type::eof) {
                    statements.push_back(consume_declaration());
                }
                consume(Token_type::right_brace, "Expected '}' after block.");

                return statements;
            }

            unique_ptr<Stmt> consume_for_statement() {
                consume(Token_type::left_paren, "Expected '(' after 'for'.");

                unique_ptr<Stmt> initializer;
                if (advance_if_match(Token_type::semicolon)) {
                    // initializer is already null
                } else if (advance_if_match(Token_type::var_)) {
                    initializer = consume_var_declaration();
                } else {
                    initializer = consume_expression_statement();
                }

                unique_ptr<Expr> condition;
                if (token_iter_->type != Token_type::semicolon) {
                    condition = consume_expression();
                }
                consume(Token_type::semicolon, "Expected ';' after loop condition.");

                unique_ptr<Expr> increment;
                if (token_iter_->type != Token_type::right_paren) {
                    increment = consume_expression();
                }
                consume(Token_type::right_paren, "Expected ')' after for clauses.");

                auto body = consume_statement();

                if (increment) {
                    vector<unique_ptr<Stmt>> stmts;
                    stmts.push_back(move(body));
                    stmts.push_back(make_unique<Expr_stmt>(move(increment)));
                    body = make_unique<Block_stmt>(move(stmts));
                }

                if (!condition) {
                    condition = make_unique<Literal_expr>(Literal{true});
                }
                body = make_unique<While_stmt>(move(condition), move(body));

                if (initializer) {
                    vector<unique_ptr<Stmt>> stmts;
                    stmts.push_back(move(initializer));
                    stmts.push_back(move(body));
                    body = make_unique<Block_stmt>(move(stmts));
                }

                return body;
            }

            unique_ptr<Stmt> consume_if_statement() {
                consume(Token_type::left_paren, "Expected '(' after 'if'.");
                auto condition = consume_expression();
                consume(Token_type::right_paren, "Expected ')' after if condition.");

                auto then_branch = consume_statement();

                unique_ptr<Stmt> else_branch;
                if (advance_if_match(Token_type::else_)) {
                    else_branch = consume_statement();
                }

                return make_unique<If_stmt>(move(condition), move(then_branch), move(else_branch));
            }

            unique_ptr<Stmt> consume_while_statement() {
                consume(Token_type::left_paren, "Expected '(' after 'while'.");
                auto condition = consume_expression();
                consume(Token_type::right_paren, "Expected ')' after condition.");

                auto body = consume_statement();

                return make_unique<While_stmt>(move(condition), move(body));
            }

            unique_ptr<Stmt> consume_print_statement() {
                auto value = consume_expression();
                consume(Token_type::semicolon, "Expected ';' after value.");

                return make_unique<Print_stmt>(move(value));
            }

            unique_ptr<Stmt> consume_return_statement() {
                unique_ptr<Expr> value;
                if (token_iter_->type != Token_type::semicolon) {
                    value = consume_expression();
                }
                consume(Token_type::semicolon, "Expected ';' after return value.");

                return make_unique<Return_stmt>(move(value));
            }

            unique_ptr<Expr> consume_expression() {
                return consume_assignment();
            }

            unique_ptr<Expr> consume_assignment() {
                auto lhs_expr = consume_or();

                if (token_iter_->type == Token_type::equal) {
                    const auto op = *move(token_iter_);
                    ++token_iter_;
                    auto rhs_expr = consume_assignment();

                    return make_unique<Assign_expr>(
                        move(*lhs_expr).lvalue_name(Parser_error{"Invalid assignment target.", op}),
                        move(rhs_expr)
                    );
                }

                return lhs_expr;
            }

            unique_ptr<Expr> consume_or() {
                auto left_expr = consume_and();

                while (token_iter_->type == Token_type::or_) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    auto right_expr = consume_and();

                    left_expr = make_unique<Logical_expr>(move(left_expr), move(op), move(right_expr));
                }

                return left_expr;
            }

            unique_ptr<Expr> consume_and() {
                auto left_expr = consume_equality();

                while (token_iter_->type == Token_type::and_) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    auto right_expr = consume_equality();

                    left_expr = make_unique<Logical_expr>(move(left_expr), move(op), move(right_expr));
                }

                return left_expr;
            }

            unique_ptr<Expr> consume_equality() {
                auto left_expr = consume_comparison();

                while (token_iter_->type == Token_type::bang_equal || token_iter_->type == Token_type::equal_equal) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    auto right_expr = consume_comparison();

                    left_expr = make_unique<Binary_expr>(move(left_expr), move(op), move(right_expr));
                }

                return left_expr;
            }

            unique_ptr<Expr> consume_comparison() {
                auto left_expr = consume_addition();

                while (
                    token_iter_->type == Token_type::greater || token_iter_->type == Token_type::greater_equal ||
                    token_iter_->type == Token_type::less || token_iter_->type == Token_type::less_equal
                ) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    auto right_expr = consume_addition();

                    left_expr = make_unique<Binary_expr>(move(left_expr), move(op), move(right_expr));
                }

                return left_expr;
            }

            unique_ptr<Expr> consume_addition() {
                auto left_expr = consume_multiplication();

                while (token_iter_->type == Token_type::minus || token_iter_->type == Token_type::plus) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    auto right_expr = consume_multiplication();

                    left_expr = make_unique<Binary_expr>(move(left_expr), move(op), move(right_expr));
                }

                return left_expr;
            }

            unique_ptr<Expr> consume_multiplication() {
                auto left_expr = consume_unary();

                while (token_iter_->type == Token_type::slash || token_iter_->type == Token_type::star) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    auto right_expr = consume_unary();

                    left_expr = make_unique<Binary_expr>(move(left_expr), move(op), move(right_expr));
                }

                return left_expr;
            }

            unique_ptr<Expr> consume_unary() {
                if (token_iter_->type == Token_type::bang || token_iter_->type == Token_type::minus) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    auto right_expr = consume_unary();

                    return make_unique<Unary_expr>(move(op), move(right_expr));
                }

                return consume_call();
            }

            unique_ptr<Expr> consume_call() {
                auto expr = consume_primary();

                while (advance_if_match(Token_type::left_paren)) {
                    expr = consume_finish_call(move(expr));
                }

                return expr;
            }

            unique_ptr<Expr> consume_finish_call(unique_ptr<Expr>&& callee) {
                vector<unique_ptr<Expr>> arguments;
                if (token_iter_->type != Token_type::right_paren) {
                    do {
                        arguments.push_back(consume_expression());
                    } while (advance_if_match(Token_type::comma));

                    if (arguments.size() > 8) {
                        throw Parser_error{"Cannot have more than 8 arguments.", *token_iter_};
                    }
                }
                auto closing_paren = consume(Token_type::right_paren, "Expected ')' after arguments.");

                return make_unique<Call_expr>(move(callee), move(closing_paren), move(arguments));
            }

            unique_ptr<Expr> consume_primary() {
                if (advance_if_match(Token_type::false_)) return make_unique<Literal_expr>(Literal{false});
                if (advance_if_match(Token_type::true_)) return make_unique<Literal_expr>(Literal{true});
                if (advance_if_match(Token_type::nil_)) return make_unique<Literal_expr>(Literal{nullptr});

                if (token_iter_->type == Token_type::number || token_iter_->type == Token_type::string) {
                    auto expr = make_unique<Literal_expr>(*((*move(token_iter_)).literal));
                    ++token_iter_;
                    return expr;
                }

                if (token_iter_->type == Token_type::identifier) {
                    auto expr = make_unique<Var_expr>(*move(token_iter_));
                    ++token_iter_;
                    return expr;
                }

                if (advance_if_match(Token_type::left_paren)) {
                    auto expr = consume_expression();
                    consume(Token_type::right_paren, "Expected ')' after expression.");

                    return make_unique<Grouping_expr>(move(expr));
                }

                throw Parser_error{"Expected expression.", *token_iter_};
            }

            Token consume(Token_type token_type, const string& error_msg) {
                if (token_iter_->type != token_type) {
                    throw Parser_error{error_msg, *token_iter_};
                }
                const auto token = *move(token_iter_);
                ++token_iter_;

                return token;
            }

            bool advance_if_match(Token_type token_type) {
                if (token_iter_->type == token_type) {
                    ++token_iter_;
                    return true;
                }

                return false;
            }

            void recover_to_synchronization_point() {
                while (token_iter_->type != Token_type::eof) {
                    // After a semicolon, we're probably finished with a statement; use it as a synchronization point
                    if (advance_if_match(Token_type::semicolon)) return;

                    // Most statements start with a keyword -- for, if, return, var, etc. Use them as synchronization
                    // points.
                    if (
                        token_iter_->type == Token_type::class_ ||
                        token_iter_->type == Token_type::fun_ ||
                        token_iter_->type == Token_type::var_ ||
                        token_iter_->type == Token_type::for_ ||
                        token_iter_->type == Token_type::if_ ||
                        token_iter_->type == Token_type::while_ ||
                        token_iter_->type == Token_type::print_ ||
                        token_iter_->type == Token_type::return_
                    ) {
                        return;
                    }

                    // Discard tokens until we find a statement boundary
                    ++token_iter_;
                }
            }
    };
}

// Exported (external linkage)
namespace motts { namespace lox {
    vector<unique_ptr<Stmt>> parse(Token_iterator&& token_iter) {
        vector<unique_ptr<Stmt>> statements;
        vector<string> parser_errors;

        Parser parser {move(token_iter)};
        while (parser.token_iter()->type != Token_type::eof) {
            try {
                statements.push_back(parser.consume_declaration());
            } catch (const Parser_error& error) {
                parser_errors.push_back(error.what());
                parser.recover_to_synchronization_point();
            }
        }
        if (parser_errors.size()) {
            throw Runtime_error{join(parser_errors, "\n")};
        }

        return statements;
    }

    Parser_error::Parser_error(const string& what, const Token& token) :
        Runtime_error {
            "[Line " + to_string(token.line) + "] Error at " + (
                token.type != Token_type::eof ? "'" + token.lexeme + "'" : "end"
            ) + ": " + what
        }
    {}
}}
