#include "parser_bc.hpp"

#include <functional>
#include <utility>

using std::function;
using std::move;
using std::string;

using boost::get;

// Allow the internal linkage section to access names
using namespace motts::lox;

// Not exported (internal linkage)
namespace {
    /*
    There's no invariant being maintained here; this class exists primarily to avoid lots of manual argument passing

    I didn't implement a Pratt Parser like in Nystrom's book. Admittedly I took the lazy way out and reused (copied) the
    structure of the original AST parser. Later I may try to make a polymorphic base parser with two specializations,
    one that emits an AST and one that emits byte code.
    */
    class Parser {
        Token_iterator token_iter_;
        function<void (const Parser_error&)> on_resumable_error_;
        Chunk chunk_;

        public:
            Parser(Token_iterator&& token_iter, function<void (const Parser_error&)>&& on_resumable_error) :
                token_iter_ {move(token_iter)},
                on_resumable_error_ {move(on_resumable_error)}
            {}

            const Token_iterator& token_iter() {
                return token_iter_;
            }

            Chunk&& chunk() && {
                return move(chunk_);
            }

            void consume_declaration() {
                try {
                    return consume_statement();
                } catch (const Parser_error& error) {
                    on_resumable_error_(error);
                    recover_to_synchronization_point();
                }
            }

            void consume_statement() {
                if (advance_if_match(Token_type::print_)) return consume_print_statement();

                consume_expression_statement();
            }

            void consume_expression_statement() {
                consume_expression();
                consume(Token_type::semicolon, "Expected ';' after expression.");
            }

            void consume_print_statement() {
                consume_expression();
                consume(Token_type::semicolon, "Expected ';' after value.");

                chunk_.code.push_back(static_cast<int>(Opcode::print));
                chunk_.lines.push_back(-1);
            }

            void consume_expression() {
                consume_assignment();
            }

            void consume_assignment() {
                consume_or();
            }

            void consume_or() {
                consume_and();
            }

            void consume_and() {
                consume_equality();
            }

            void consume_equality() {
                consume_comparison();
            }

            void consume_comparison() {
                consume_addition();
            }

            void consume_addition() {
                consume_multiplication();

                while (token_iter_->type == Token_type::minus || token_iter_->type == Token_type::plus) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    consume_multiplication();

                    chunk_.code.push_back(static_cast<int>([&] () {
                        switch (op.type) {
                            case Token_type::plus: return Opcode::add;
                            case Token_type::minus: return Opcode::subtract;
                        }
                    }()));
                    chunk_.lines.push_back(op.line);
                }
            }

            void consume_multiplication() {
                consume_unary();

                while (token_iter_->type == Token_type::slash || token_iter_->type == Token_type::star) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    consume_unary();

                    chunk_.code.push_back(static_cast<int>([&] () {
                        switch (op.type) {
                            case Token_type::star: return Opcode::multiply;
                            case Token_type::slash: return Opcode::divide;
                        }
                    }()));
                    chunk_.lines.push_back(op.line);
                }
            }

            void consume_unary() {
                if (token_iter_->type == Token_type::minus) {
                    auto op = *move(token_iter_);
                    ++token_iter_;
                    consume_unary();

                    chunk_.code.push_back(static_cast<int>(Opcode::negate));
                    chunk_.lines.push_back(op.line);
                }

                consume_call();
            }

            void consume_call() {
                consume_primary();
            }

            void consume_primary() {
                if (token_iter_->type == Token_type::number) {
                    chunk_.code.push_back(static_cast<int>(Opcode::constant));
                    chunk_.constants.push_back(get<double>(token_iter_->literal->value));
                    chunk_.lines.push_back(-1);

                    chunk_.code.push_back(chunk_.constants.cend() - chunk_.constants.cbegin() - 1);
                    chunk_.lines.push_back(-1);

                    ++token_iter_;
                    return;
                }

                if (advance_if_match(Token_type::left_paren)) {
                    consume_expression();
                    consume(Token_type::right_paren, "Expected ')' after expression.");
                    return;
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
    Chunk parse_bc(Token_iterator&& token_iter) {
        string parser_errors;
        Parser parser {move(token_iter), [&] (const Parser_error& error) {
            if (!parser_errors.empty()) {
                parser_errors += '\n';
            }
            parser_errors += error.what();
        }};

        while (parser.token_iter()->type != Token_type::eof) {
            parser.consume_declaration();
        }

        if (!parser_errors.empty()) {
            throw Runtime_error{parser_errors};
        }

        return move(parser).chunk();
    }
}}
