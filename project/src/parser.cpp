#include "parser.hpp"

#include <functional>
#include <utility>

#include <gc.h>

#include "expression_impls.hpp"
#include "statement_impls.hpp"

using std::function;
using std::move;
using std::string;
using std::to_string;
using std::vector;

using boost::optional;

// Allow the internal linkage section to access names
using namespace motts::lox;

// Not exported (internal linkage)
namespace {
    // There's no invariant being maintained here; this class exists primarily to avoid lots of manual argument passing
    class Parser {
        Token_iterator token_iter_;
        function<void (const Parser_error&)> on_resumable_error_;

        public:
            Parser(Token_iterator&& token_iter, function<void (const Parser_error&)>&& on_resumable_error) :
                token_iter_ {move(token_iter)},
                on_resumable_error_ {move(on_resumable_error)}
            {}

            const Token_iterator& token_iter() {
                return token_iter_;
            }

            const Stmt* consume_declaration() {
                try {
                    if (advance_if_match(Token_type::class_)) return consume_class_declaration();
                    if (advance_if_match(Token_type::fun_)) return consume_function_declaration();
                    if (advance_if_match(Token_type::var_)) return consume_var_declaration();

                    return consume_statement();
                } catch (const Parser_error& error) {
                    on_resumable_error_(error);
                    recover_to_synchronization_point();

                    return {};
                }
            }

            const Stmt* consume_var_declaration() {
                auto var_name = consume(Token_type::identifier, "Expected variable name.");

                const Expr* initializer {};
                if (advance_if_match(Token_type::equal)) {
                    initializer = consume_expression();
                }

                consume(Token_type::semicolon, "Expected ';' after variable declaration.");

                return new (GC_MALLOC(sizeof(Var_stmt))) Var_stmt{move(var_name), move(initializer)};
            }

            const Stmt* consume_class_declaration() {
                auto name = consume(Token_type::identifier, "Expected class name.");

                Var_expr* superclass {};
                if (advance_if_match(Token_type::less)) {
                    auto super_name = consume(Token_type::identifier, "Expected superclass name.");
                    superclass = new (GC_MALLOC(sizeof(Var_expr))) Var_expr{move(super_name)};
                }

                consume(Token_type::left_brace, "Expected '{' before class body.");

                vector<const Function_stmt*> methods;
                while (token_iter_->type != Token_type::right_brace && token_iter_->type != Token_type::eof) {
                    methods.push_back(consume_function_declaration());
                }

                consume(Token_type::right_brace, "Expected '}' after class body.");

                return new (GC_MALLOC(sizeof(Class_stmt))) Class_stmt{move(name), move(superclass), move(methods)};
            }

            const Function_stmt* consume_function_declaration() {
                auto name = consume(Token_type::identifier, "Expected function name.");
                return new (GC_MALLOC(sizeof(Function_stmt))) Function_stmt{consume_finish_function(move(name))};
            }

            const Function_expr* consume_function_expression() {
                optional<Token> name;
                if (token_iter_->type == Token_type::identifier) {
                    name = *move(token_iter_);
                    ++token_iter_;
                }

                return consume_finish_function(std::move(name));
            }

            const Function_expr* consume_finish_function(optional<Token>&& name) {
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

                return new (GC_MALLOC(sizeof(Function_expr))) Function_expr{std::move(name), move(parameters), move(body)};
            }

            const Stmt* consume_statement() {
                if (advance_if_match(Token_type::for_)) return consume_for_statement();
                if (advance_if_match(Token_type::if_)) return consume_if_statement();
                if (advance_if_match(Token_type::print_)) return consume_print_statement();
                if (token_iter_->type == Token_type::return_) {
                    auto keyword = *move(token_iter_);
                    ++token_iter_;

                    return consume_return_statement(move(keyword));
                }
                if (advance_if_match(Token_type::while_)) return consume_while_statement();
                if (advance_if_match(Token_type::left_brace)) return new (GC_MALLOC(sizeof(Block_stmt))) Block_stmt{consume_block_statement()};

                return consume_expression_statement();
            }

            const Stmt* consume_expression_statement() {
                auto expr = consume_expression();
                consume(Token_type::semicolon, "Expected ';' after expression.");

                return new (GC_MALLOC(sizeof(Expr_stmt))) Expr_stmt{move(expr)};
            }

            vector<const Stmt*> consume_block_statement() {
                vector<const Stmt*> statements;
                while (token_iter_->type != Token_type::right_brace && token_iter_->type != Token_type::eof) {
                    statements.push_back(consume_declaration());
                }
                consume(Token_type::right_brace, "Expected '}' after block.");

                return statements;
            }

            const Stmt* consume_for_statement() {
                consume(Token_type::left_paren, "Expected '(' after 'for'.");

                const Stmt* initializer {};
                if (advance_if_match(Token_type::semicolon)) {
                    // initializer is already null
                } else if (advance_if_match(Token_type::var_)) {
                    initializer = consume_var_declaration();
                } else {
                    initializer = consume_expression_statement();
                }

                const Expr* condition {};
                if (token_iter_->type != Token_type::semicolon) {
                    condition = consume_expression();
                }
                consume(Token_type::semicolon, "Expected ';' after loop condition.");

                const Expr* increment {};
                if (token_iter_->type != Token_type::right_paren) {
                    increment = consume_expression();
                }
                consume(Token_type::right_paren, "Expected ')' after for clauses.");

                auto body = consume_statement();

                if (increment) {
                    vector<const Stmt*> stmts;
                    stmts.push_back(move(body));
                    stmts.push_back(new (GC_MALLOC(sizeof(Expr_stmt))) Expr_stmt{move(increment)});
                    body = new (GC_MALLOC(sizeof(Block_stmt))) Block_stmt{move(stmts)};
                }

                if (!condition) {
                    condition = new (GC_MALLOC(sizeof(Literal_expr))) Literal_expr{Literal{true}};
                }
                body = new (GC_MALLOC(sizeof(While_stmt))) While_stmt{move(condition), move(body)};

                if (initializer) {
                    vector<const Stmt*> stmts;
                    stmts.push_back(move(initializer));
                    stmts.push_back(move(body));
                    body = new (GC_MALLOC(sizeof(Block_stmt))) Block_stmt{move(stmts)};
                }

                return body;
            }

            const Stmt* consume_if_statement() {
                consume(Token_type::left_paren, "Expected '(' after 'if'.");
                auto condition = consume_expression();
                consume(Token_type::right_paren, "Expected ')' after if condition.");

                auto then_branch = consume_statement();

                const Stmt* else_branch {};
                if (advance_if_match(Token_type::else_)) {
                    else_branch = consume_statement();
                }

                return new (GC_MALLOC(sizeof(If_stmt))) If_stmt{move(condition), move(then_branch), move(else_branch)};
            }

            const Stmt* consume_while_statement() {
                consume(Token_type::left_paren, "Expected '(' after 'while'.");
                auto condition = consume_expression();
                consume(Token_type::right_paren, "Expected ')' after condition.");

                auto body = consume_statement();

                return new (GC_MALLOC(sizeof(While_stmt))) While_stmt{move(condition), move(body)};
            }

            const Stmt* consume_print_statement() {
                auto value = consume_expression();
                consume(Token_type::semicolon, "Expected ';' after value.");

                return new (GC_MALLOC(sizeof(Print_stmt))) Print_stmt{move(value)};
            }

            const Stmt* consume_return_statement(Token&& keyword) {
                const Expr* value {};
                if (token_iter_->type != Token_type::semicolon) {
                    value = consume_expression();
                }
                consume(Token_type::semicolon, "Expected ';' after return value.");

                return new (GC_MALLOC(sizeof(Return_stmt))) Return_stmt{move(keyword), move(value)};
            }

            const Expr* consume_expression() {
                return consume_assignment();
            }

            const Expr* consume_assignment() {
                auto lhs_expr = consume_or();

                if (token_iter_->type == Token_type::equal) {
                    const auto op = *move(token_iter_);
                    ++token_iter_;
                    auto rhs_expr = consume_assignment();

                    // The lhs might be a var expression or it might be an object get expression, and which it is
                    // affects which type we need to instantiate. Rather than using RTTI and dynamic_cast, instead rely
                    // on polymorphism and virtual calls to do the right thing for each type. A virtual call through
                    // Var_expr will make and return an Assign_expr, and a virtual call through a Get_expr will make and
                    // return a Set_expr.
                    return lhs_expr->make_assignment_expression(
                        move(lhs_expr),
                        move(rhs_expr),
                        Parser_error{"Invalid assignment target.", op}
                    );
                }

                return lhs_expr;
            }

            const Expr* consume_or() {
                auto left_expr = consume_and();

                while (token_iter_->type == Token_type::or_) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    auto right_expr = consume_and();

                    left_expr = new (GC_MALLOC(sizeof(Logical_expr))) Logical_expr{move(left_expr), move(op), move(right_expr)};
                }

                return left_expr;
            }

            const Expr* consume_and() {
                auto left_expr = consume_equality();

                while (token_iter_->type == Token_type::and_) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    auto right_expr = consume_equality();

                    left_expr = new (GC_MALLOC(sizeof(Logical_expr))) Logical_expr{move(left_expr), move(op), move(right_expr)};
                }

                return left_expr;
            }

            const Expr* consume_equality() {
                auto left_expr = consume_comparison();

                while (token_iter_->type == Token_type::bang_equal || token_iter_->type == Token_type::equal_equal) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    auto right_expr = consume_comparison();

                    left_expr = new (GC_MALLOC(sizeof(Binary_expr))) Binary_expr{move(left_expr), move(op), move(right_expr)};
                }

                return left_expr;
            }

            const Expr* consume_comparison() {
                auto left_expr = consume_addition();

                while (
                    token_iter_->type == Token_type::greater || token_iter_->type == Token_type::greater_equal ||
                    token_iter_->type == Token_type::less || token_iter_->type == Token_type::less_equal
                ) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    auto right_expr = consume_addition();

                    left_expr = new (GC_MALLOC(sizeof(Binary_expr))) Binary_expr{move(left_expr), move(op), move(right_expr)};
                }

                return left_expr;
            }

            const Expr* consume_addition() {
                auto left_expr = consume_multiplication();

                while (token_iter_->type == Token_type::minus || token_iter_->type == Token_type::plus) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    auto right_expr = consume_multiplication();

                    left_expr = new (GC_MALLOC(sizeof(Binary_expr))) Binary_expr{move(left_expr), move(op), move(right_expr)};
                }

                return left_expr;
            }

            const Expr* consume_multiplication() {
                auto left_expr = consume_unary();

                while (token_iter_->type == Token_type::slash || token_iter_->type == Token_type::star) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    auto right_expr = consume_unary();

                    left_expr = new (GC_MALLOC(sizeof(Binary_expr))) Binary_expr{move(left_expr), move(op), move(right_expr)};
                }

                return left_expr;
            }

            const Expr* consume_unary() {
                if (token_iter_->type == Token_type::bang || token_iter_->type == Token_type::minus) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    auto right_expr = consume_unary();

                    return new (GC_MALLOC(sizeof(Unary_expr))) Unary_expr{move(op), move(right_expr)};
                }

                return consume_call();
            }

            const Expr* consume_call() {
                auto expr = consume_primary();

                while (true) {
                    if (advance_if_match(Token_type::left_paren)) {
                        expr = consume_finish_call(move(expr));
                        continue;
                    }

                    if (advance_if_match(Token_type::dot)) {
                        auto name = consume(Token_type::identifier, "Expected property name after '.'.");
                        expr = new (GC_MALLOC(sizeof(Get_expr))) Get_expr{move(expr), move(name)};
                        continue;
                    }

                    break;
                }

                return expr;
            }

            const Expr* consume_finish_call(const Expr* callee) {
                vector<const Expr*> arguments;
                if (token_iter_->type != Token_type::right_paren) {
                    do {
                        arguments.push_back(consume_expression());
                    } while (advance_if_match(Token_type::comma));

                    if (arguments.size() > 8) {
                        throw Parser_error{"Cannot have more than 8 arguments.", *token_iter_};
                    }
                }
                auto closing_paren = consume(Token_type::right_paren, "Expected ')' after arguments.");

                return new (GC_MALLOC(sizeof(Call_expr))) Call_expr{move(callee), move(closing_paren), move(arguments)};
            }

            const Expr* consume_primary() {
                if (advance_if_match(Token_type::false_)) return new (GC_MALLOC(sizeof(Literal_expr))) Literal_expr{Literal{false}};
                if (advance_if_match(Token_type::true_)) return new (GC_MALLOC(sizeof(Literal_expr))) Literal_expr{Literal{true}};
                if (advance_if_match(Token_type::nil_)) return new (GC_MALLOC(sizeof(Literal_expr))) Literal_expr{Literal{nullptr}};

                if (token_iter_->type == Token_type::number || token_iter_->type == Token_type::string) {
                    auto expr = new (GC_MALLOC(sizeof(Literal_expr))) Literal_expr{*((*move(token_iter_)).literal)};
                    ++token_iter_;
                    return expr;
                }

                if (token_iter_->type == Token_type::super_) {
                    auto keyword = *move(token_iter_);
                    ++token_iter_;
                    consume(Token_type::dot, "Expected '.' after 'super'.");
                    auto method = consume(Token_type::identifier, "Expected superclass method name.");
                    return new (GC_MALLOC(sizeof(Super_expr))) Super_expr{move(keyword), move(method)};
                }

                if (token_iter_->type == Token_type::this_) {
                    auto keyword = *move(token_iter_);
                    ++token_iter_;
                    return new (GC_MALLOC(sizeof(This_expr))) This_expr{move(keyword)};
                }

                if (advance_if_match(Token_type::fun_)) {
                    return consume_function_expression();
                }

                if (token_iter_->type == Token_type::identifier) {
                    auto expr = new (GC_MALLOC(sizeof(Var_expr))) Var_expr{*move(token_iter_)};
                    ++token_iter_;
                    return expr;
                }

                if (advance_if_match(Token_type::left_paren)) {
                    auto expr = consume_expression();
                    consume(Token_type::right_paren, "Expected ')' after expression.");
                    return new (GC_MALLOC(sizeof(Grouping_expr))) Grouping_expr{move(expr)};
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

                    // Most statements start with a keyword -- for, if, return, var, etc. Use them
                    // as synchronization points.
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
    vector<const Stmt*> parse(Token_iterator&& token_iter) {
        vector<const Stmt*> statements;

        string parser_errors;
        Parser parser {move(token_iter), [&] (const Parser_error& error) {
            if (!parser_errors.empty()) {
                parser_errors += '\n';
            }
            parser_errors += error.what();
        }};
        while (parser.token_iter()->type != Token_type::eof) {
            auto declaration = parser.consume_declaration();
            if (declaration) {
                statements.push_back(move(declaration));
            }
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
