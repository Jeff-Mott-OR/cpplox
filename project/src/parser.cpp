#include "parser.hpp"

#include <functional>
#include <utility>

#include "expression_impls.hpp"
#include "statement_impls.hpp"

using std::function;
using std::move;
using std::string;
using std::to_string;
using std::vector;

using boost::optional;
using deferred_heap_t = gcpp::deferred_heap;
using gcpp::deferred_ptr;

// Allow the internal linkage section to access names
using namespace motts::lox;

// Not exported (internal linkage)
namespace {
    // There's no invariant being maintained here; this exists primarily to avoid lots of manual argument passing
    struct Parser {
        deferred_heap_t& deferred_heap;
        Token_iterator& token_iter;
        function<void(const Parser_error&)> on_resumable_error;

        deferred_ptr<const Stmt> consume_declaration() {
            try {
                if (advance_if_match(Token_type::class_)) return consume_class_declaration();
                if (advance_if_match(Token_type::fun_)) return consume_function_declaration();
                if (advance_if_match(Token_type::var_)) return consume_var_declaration();
                return consume_statement();
            } catch (const Parser_error& error) {
                on_resumable_error(error);
                recover_to_synchronization_point();
                return {};
            }
        }

        deferred_ptr<const Stmt> consume_var_declaration() {
            auto var_name = consume(Token_type::identifier, "Expected variable name.");

            deferred_ptr<const Expr> initializer;
            if (advance_if_match(Token_type::equal)) {
                initializer = consume_expression();
            }

            consume(Token_type::semicolon, "Expected ';' after variable declaration.");

            return deferred_heap.make<Var_stmt>(move(var_name), move(initializer));
        }

        deferred_ptr<const Stmt> consume_class_declaration() {
            auto name = consume(Token_type::identifier, "Expected class name.");

            deferred_ptr<const Var_expr> superclass;
            if (advance_if_match(Token_type::less)) {
                auto super_name = consume(Token_type::identifier, "Expected superclass name.");
                superclass = deferred_heap.make<Var_expr>(move(super_name));
            }

            consume(Token_type::left_brace, "Expected '{' before class body.");

            vector<deferred_ptr<const Function_stmt>> methods;
            while (token_iter->type != Token_type::right_brace && token_iter->type != Token_type::eof) {
                methods.push_back(consume_function_declaration());
            }

            consume(Token_type::right_brace, "Expected '}' after class body.");

            return deferred_heap.make<Class_stmt>(move(name), move(superclass), move(methods));
        }

        deferred_ptr<const Function_stmt> consume_function_declaration() {
            auto name = consume(Token_type::identifier, "Expected function name.");
            return deferred_heap.make<Function_stmt>(consume_finish_function(move(name)));
        }

        deferred_ptr<const Function_expr> consume_function_expression() {
            optional<Token> name;
            if (token_iter->type == Token_type::identifier) {
                name = *move(token_iter);
                ++token_iter;
            }

            return consume_finish_function(std::move(name));
        }

        deferred_ptr<const Function_expr> consume_finish_function(optional<Token>&& name) {
            consume(Token_type::left_paren, "Expected '(' after function name.");
            vector<Token> parameters;
            if (token_iter->type != Token_type::right_paren) {
                do {
                    parameters.push_back(consume(Token_type::identifier, "Expected parameter name."));
                } while (advance_if_match(Token_type::comma));

                if (parameters.size() > 8) {
                    throw Parser_error{"Cannot have more than 8 parameters.", *token_iter};
                }
            }
            consume(Token_type::right_paren, "Expected ')' after parameters.");

            consume(Token_type::left_brace, "Expected '{' before function body.");
            auto body = consume_block_statement();

            return deferred_heap.make<Function_expr>(std::move(name), move(parameters), move(body));
        }

        deferred_ptr<const Stmt> consume_statement() {
            if (advance_if_match(Token_type::for_)) return consume_for_statement();
            if (advance_if_match(Token_type::if_)) return consume_if_statement();
            if (advance_if_match(Token_type::print_)) return consume_print_statement();
            if (token_iter->type == Token_type::return_) {
                auto keyword = *move(token_iter);
                ++token_iter;
                return consume_return_statement(move(keyword));
            }
            if (advance_if_match(Token_type::while_)) return consume_while_statement();
            if (advance_if_match(Token_type::left_brace)) return deferred_heap.make<Block_stmt>(consume_block_statement());
            if (advance_if_match(Token_type::break_)) return consume_break_statement();
            if (advance_if_match(Token_type::continue_)) return consume_continue_statement();
            return consume_expression_statement();
        }

        deferred_ptr<const Stmt> consume_expression_statement() {
            auto expr = consume_expression();
            consume(Token_type::semicolon, "Expected ';' after expression.");

            return deferred_heap.make<Expr_stmt>(move(expr));
        }

        vector<deferred_ptr<const Stmt>> consume_block_statement() {
            vector<deferred_ptr<const Stmt>> statements;
            while (token_iter->type != Token_type::right_brace && token_iter->type != Token_type::eof) {
                statements.push_back(consume_declaration());
            }
            consume(Token_type::right_brace, "Expected '}' after block.");

            return statements;
        }

        deferred_ptr<const Stmt> consume_for_statement() {
            consume(Token_type::left_paren, "Expected '(' after 'for'.");

            deferred_ptr<const Stmt> initializer;
            if (advance_if_match(Token_type::semicolon)) {
                // initializer is already null
            } else if (advance_if_match(Token_type::var_)) {
                initializer = consume_var_declaration();
            } else {
                initializer = consume_expression_statement();
            }

            deferred_ptr<const Expr> condition {
                token_iter->type != Token_type::semicolon ?
                    consume_expression() :
                    deferred_heap.make<Literal_expr>(Literal{true})
            };
            consume(Token_type::semicolon, "Expected ';' after loop condition.");

            deferred_ptr<const Expr> increment {
                token_iter->type != Token_type::right_paren ?
                    consume_expression() :
                    deferred_heap.make<Literal_expr>(Literal{})
            };

            consume(Token_type::right_paren, "Expected ')' after for clauses.");
            auto body = consume_statement();
            body = deferred_heap.make<For_stmt>(move(condition), move(increment), move(body));
            if (initializer) {
                body = deferred_heap.make<Block_stmt>(vector<deferred_ptr<const Stmt>>{move(initializer), move(body)});
            }

            return body;
        }

        deferred_ptr<const Stmt> consume_if_statement() {
            consume(Token_type::left_paren, "Expected '(' after 'if'.");
            auto condition = consume_expression();
            consume(Token_type::right_paren, "Expected ')' after if condition.");

            auto then_branch = consume_statement();

            deferred_ptr<const Stmt> else_branch;
            if (advance_if_match(Token_type::else_)) {
                else_branch = consume_statement();
            }

            return deferred_heap.make<If_stmt>(move(condition), move(then_branch), move(else_branch));
        }

        deferred_ptr<const Stmt> consume_while_statement() {
            consume(Token_type::left_paren, "Expected '(' after 'while'.");
            auto condition = consume_expression();
            consume(Token_type::right_paren, "Expected ')' after condition.");

            auto body = consume_statement();

            return deferred_heap.make<While_stmt>(move(condition), move(body));
        }

        deferred_ptr<const Stmt> consume_print_statement() {
            auto value = consume_expression();
            consume(Token_type::semicolon, "Expected ';' after value.");

            return deferred_heap.make<Print_stmt>(move(value));
        }

        deferred_ptr<const Stmt> consume_return_statement(Token&& keyword) {
            deferred_ptr<const Expr> value;
            if (token_iter->type != Token_type::semicolon) {
                value = consume_expression();
            }
            consume(Token_type::semicolon, "Expected ';' after return value.");

            return deferred_heap.make<Return_stmt>(move(keyword), move(value));
        }

        deferred_ptr<const Stmt> consume_break_statement() {
            consume(Token_type::semicolon, "Expected ';' after 'break'.");
            return deferred_heap.make<Break_stmt>();
        }

        deferred_ptr<const Stmt> consume_continue_statement() {
            consume(Token_type::semicolon, "Expected ';' after 'continue'.");
            return deferred_heap.make<Continue_stmt>();
        }

        deferred_ptr<const Expr> consume_expression() {
            return consume_assignment();
        }

        deferred_ptr<const Expr> consume_assignment() {
            auto left_expr = consume_or();

            if (token_iter->type == Token_type::equal) {
                const auto op = *move(token_iter);
                ++token_iter;
                auto right_expr = consume_assignment();

                // The lhs might be a var expression or it might be an object get expression, and which it is
                // affects which type we need to instantiate. Rather than using RTTI and dynamic_cast, instead rely
                // on polymorphism and virtual calls to do the right thing for each type. A virtual call through
                // Var_expr will make and return an Assign_expr, and a virtual call through a Get_expr will make and
                // return a Set_expr.
                return left_expr->make_assignment_expression(
                    deferred_heap,
                    move(left_expr),
                    move(right_expr),
                    Parser_error{"Invalid assignment target.", op}
                );
            }

            return left_expr;
        }

        deferred_ptr<const Expr> consume_or() {
            auto left_expr = consume_and();

            while (token_iter->type == Token_type::or_) {
                auto op = *move(token_iter);
                ++token_iter;
                auto right_expr = consume_and();

                left_expr = deferred_heap.make<Logical_expr>(move(left_expr), move(op), move(right_expr));
            }

            return left_expr;
        }

        deferred_ptr<const Expr> consume_and() {
            auto left_expr = consume_equality();

            while (token_iter->type == Token_type::and_) {
                auto op = *move(token_iter);
                ++token_iter;
                auto right_expr = consume_equality();

                left_expr = deferred_heap.make<Logical_expr>(move(left_expr), move(op), move(right_expr));
            }

            return left_expr;
        }

        deferred_ptr<const Expr> consume_equality() {
            auto left_expr = consume_comparison();

            while (token_iter->type == Token_type::bang_equal || token_iter->type == Token_type::equal_equal) {
                auto op = *move(token_iter);
                ++token_iter;
                auto right_expr = consume_comparison();

                left_expr = deferred_heap.make<Binary_expr>(move(left_expr), move(op), move(right_expr));
            }

            return left_expr;
        }

        deferred_ptr<const Expr> consume_comparison() {
            auto left_expr = consume_addition();

            while (
                token_iter->type == Token_type::greater || token_iter->type == Token_type::greater_equal ||
                token_iter->type == Token_type::less || token_iter->type == Token_type::less_equal
            ) {
                auto op = *move(token_iter);
                ++token_iter;
                auto right_expr = consume_addition();

                left_expr = deferred_heap.make<Binary_expr>(move(left_expr), move(op), move(right_expr));
            }

            return left_expr;
        }

        deferred_ptr<const Expr> consume_addition() {
            auto left_expr = consume_multiplication();

            while (token_iter->type == Token_type::minus || token_iter->type == Token_type::plus) {
                auto op = *move(token_iter);
                ++token_iter;
                auto right_expr = consume_multiplication();

                left_expr = deferred_heap.make<Binary_expr>(move(left_expr), move(op), move(right_expr));
            }

            return left_expr;
        }

        deferred_ptr<const Expr> consume_multiplication() {
            auto left_expr = consume_unary();

            while (token_iter->type == Token_type::slash || token_iter->type == Token_type::star) {
                auto op = *move(token_iter);
                ++token_iter;
                auto right_expr = consume_unary();

                left_expr = deferred_heap.make<Binary_expr>(move(left_expr), move(op), move(right_expr));
            }

            return left_expr;
        }

        deferred_ptr<const Expr> consume_unary() {
            if (token_iter->type == Token_type::bang || token_iter->type == Token_type::minus) {
                auto op = *move(token_iter);
                ++token_iter;
                auto right_expr = consume_unary();

                return deferred_heap.make<Unary_expr>(move(op), move(right_expr));
            }

            return consume_call();
        }

        deferred_ptr<const Expr> consume_call() {
            auto expr = consume_primary();

            while (true) {
                if (advance_if_match(Token_type::left_paren)) {
                    expr = consume_finish_call(move(expr));
                    continue;
                }

                if (advance_if_match(Token_type::dot)) {
                    auto name = consume(Token_type::identifier, "Expected property name after '.'.");
                    expr = deferred_heap.make<Get_expr>(move(expr), move(name));
                    continue;
                }

                break;
            }

            return expr;
        }

        deferred_ptr<const Expr> consume_finish_call(deferred_ptr<const Expr>&& callee) {
            vector<deferred_ptr<const Expr>> arguments;
            if (token_iter->type != Token_type::right_paren) {
                do {
                    arguments.push_back(consume_expression());
                } while (advance_if_match(Token_type::comma));

                if (arguments.size() > 8) {
                    throw Parser_error{"Cannot have more than 8 arguments.", *token_iter};
                }
            }
            auto closing_paren = consume(Token_type::right_paren, "Expected ')' after arguments.");

            return deferred_heap.make<Call_expr>(move(callee), move(closing_paren), move(arguments));
        }

        deferred_ptr<const Expr> consume_primary() {
            if (advance_if_match(Token_type::false_)) return deferred_heap.make<Literal_expr>(Literal{false});
            if (advance_if_match(Token_type::true_)) return deferred_heap.make<Literal_expr>(Literal{true});
            if (advance_if_match(Token_type::nil_)) return deferred_heap.make<Literal_expr>(Literal{nullptr});

            if (token_iter->type == Token_type::number || token_iter->type == Token_type::string) {
                auto expr = deferred_heap.make<Literal_expr>(*((*move(token_iter)).literal));
                ++token_iter;
                return expr;
            }

            if (token_iter->type == Token_type::super_) {
                auto keyword = *move(token_iter);
                ++token_iter;
                consume(Token_type::dot, "Expected '.' after 'super'.");
                auto method = consume(Token_type::identifier, "Expected superclass method name.");
                return deferred_heap.make<Super_expr>(move(keyword), move(method));
            }

            if (token_iter->type == Token_type::this_) {
                auto keyword = *move(token_iter);
                ++token_iter;
                return deferred_heap.make<This_expr>(move(keyword));
            }

            if (advance_if_match(Token_type::fun_)) {
                return consume_function_expression();
            }

            if (token_iter->type == Token_type::identifier) {
                auto expr = deferred_heap.make<Var_expr>(*move(token_iter));
                ++token_iter;
                return expr;
            }

            if (advance_if_match(Token_type::left_paren)) {
                auto expr = consume_expression();
                consume(Token_type::right_paren, "Expected ')' after expression.");
                return deferred_heap.make<Grouping_expr>(move(expr));
            }

            throw Parser_error{"Expected expression.", *token_iter};
        }

        Token consume(Token_type token_type, const string& error_msg) {
            if (token_iter->type != token_type) {
                throw Parser_error{error_msg, *token_iter};
            }
            const auto token = *move(token_iter);
            ++token_iter;

            return token;
        }

        bool advance_if_match(Token_type token_type) {
            if (token_iter->type == token_type) {
                ++token_iter;
                return true;
            }

            return false;
        }

        void recover_to_synchronization_point() {
            while (token_iter->type != Token_type::eof) {
                // After a semicolon, we're probably finished with a statement; use it as a synchronization point
                if (advance_if_match(Token_type::semicolon)) return;

                // Most statements start with a keyword -- for, if, return, var, etc. Use them
                // as synchronization points.
                if (
                    token_iter->type == Token_type::class_ ||
                    token_iter->type == Token_type::fun_ ||
                    token_iter->type == Token_type::var_ ||
                    token_iter->type == Token_type::for_ ||
                    token_iter->type == Token_type::if_ ||
                    token_iter->type == Token_type::while_ ||
                    token_iter->type == Token_type::print_ ||
                    token_iter->type == Token_type::return_
                ) {
                    return;
                }

                // Discard tokens until we find a statement boundary
                ++token_iter;
            }
        }
    };
}

// Exported (external linkage)
namespace motts { namespace lox {
    vector<deferred_ptr<const Stmt>> parse(deferred_heap_t& deferred_heap, Token_iterator&& token_iter) {
        vector<deferred_ptr<const Stmt>> statements;

        string parser_errors;
        Parser parser {
            deferred_heap,
            token_iter,
            [&] (const Parser_error& error) {
                if (!parser_errors.empty()) {
                    parser_errors += '\n';
                }
                parser_errors += error.what();
            }
        };

        while (parser.token_iter->type != Token_type::eof) {
            statements.push_back(parser.consume_declaration());
        }
        if (!parser_errors.empty()) {
            throw Runtime_error{parser_errors};
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
