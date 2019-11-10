#include "compiler.hpp"

#include <functional>
#include <utility>
#include <vector>

#include <boost/lexical_cast.hpp>

#include "debug.hpp"

using std::function;
using std::move;
using std::runtime_error;
using std::string;
using std::to_string;
using std::vector;

using boost::lexical_cast;

using namespace motts::lox;

namespace {
    enum class Precedence {
        none,
        assignment,  // =
        or_,         // or
        and_,        // and
        equality,    // == !=
        comparison,  // < > <= >=
        term,        // + -
        factor,      // * /
        unary,       // ! -
        call,        // . () []
        primary
    };

    struct Compiler {
        struct Parse_rule {
            void (Compiler::*prefix)(bool);
            void (Compiler::*infix)(bool);
            Precedence precedence;
        };

        const vector<Parse_rule> rules_ {
            { &Compiler::compile_grouping, nullptr,                    Precedence::none },   // LEFT_PAREN
            { nullptr,                     nullptr,                    Precedence::none },   // RIGHT_PAREN
            { nullptr,                     nullptr,                    Precedence::none },   // LEFT_BRACE
            { nullptr,                     nullptr,                    Precedence::none },   // RIGHT_BRACE
            { nullptr,                     nullptr,                    Precedence::none },   // COMMA
            { nullptr,                     nullptr,                    Precedence::none },   // DOT
            { &Compiler::compile_unary,    &Compiler::compile_binary,  Precedence::term },   // MINUS
            { nullptr,                     &Compiler::compile_binary,  Precedence::term },   // PLUS
            { nullptr,                     nullptr,                    Precedence::none },   // SEMICOLON
            { nullptr,                     &Compiler::compile_binary,  Precedence::factor }, // SLASH
            { nullptr,                     &Compiler::compile_binary,  Precedence::factor }, // STAR
            { &Compiler::compile_unary,    nullptr,                    Precedence::none },   // BANG
            { nullptr,                     &Compiler::compile_binary,  Precedence::equality },   // BANG_EQUAL
            { nullptr,                     nullptr,                    Precedence::none },   // EQUAL
            { nullptr,                     &Compiler::compile_binary,  Precedence::equality },   // EQUAL_EQUAL
            { nullptr,                     &Compiler::compile_binary,  Precedence::comparison },   // GREATER
            { nullptr,                     &Compiler::compile_binary,  Precedence::comparison },   // GREATER_EQUAL
            { nullptr,                     &Compiler::compile_binary,  Precedence::comparison },   // LESS
            { nullptr,                     &Compiler::compile_binary,  Precedence::comparison },   // LESS_EQUAL
            { &Compiler::compile_variable, nullptr,                    Precedence::none },   // IDENTIFIER
            { &Compiler::compile_string,   nullptr,                    Precedence::none },   // STRING
            { &Compiler::compile_number,   nullptr,                    Precedence::none },   // NUMBER
            { nullptr,                     nullptr,                    Precedence::none },   // AND
            { nullptr,                     nullptr,                    Precedence::none },   // CLASS
            { nullptr,                     nullptr,                    Precedence::none },   // ELSE
            { &Compiler::compile_literal,  nullptr,                    Precedence::none },   // FALSE
            { nullptr,                     nullptr,                    Precedence::none },   // FOR
            { nullptr,                     nullptr,                    Precedence::none },   // FUN
            { nullptr,                     nullptr,                    Precedence::none },   // IF
            { &Compiler::compile_literal,  nullptr,                    Precedence::none },   // NIL
            { nullptr,                     nullptr,                    Precedence::none },   // OR
            { nullptr,                     nullptr,                    Precedence::none },   // PRINT
            { nullptr,                     nullptr,                    Precedence::none },   // RETURN
            { nullptr,                     nullptr,                    Precedence::none },   // SUPER
            { nullptr,                     nullptr,                    Precedence::none },   // THIS
            { &Compiler::compile_literal,  nullptr,                    Precedence::none },   // TRUE
            { nullptr,                     nullptr,                    Precedence::none },   // VAR
            { nullptr,                     nullptr,                    Precedence::none },   // WHILE
            { nullptr,                     nullptr,                    Precedence::none },   // ERROR
            { nullptr,                     nullptr,                    Precedence::none },   // EOF
        };

        const Parse_rule& find_parse_rule(Token_type type) {
            return rules_.at(static_cast<int>(type));
        }

        Token_iterator token_iter_;
        Chunk chunk_;
        function<void(const Compiler_error&)> on_resumable_error_;

        Compiler(const string& source, function<void(const Compiler_error&)> on_resumable_error) :
            token_iter_ {source},
            on_resumable_error_ {move(on_resumable_error)}
        {}

        void compile_precedence_or_higher(Precedence min_precedence) {
            const auto can_assign = min_precedence <= Precedence::assignment;

            const auto prefix_rule = find_parse_rule(token_iter_->type).prefix;
            if (!prefix_rule) {
                throw Compiler_error{*token_iter_, "Expected expression."};
            }
            (this->*prefix_rule)(can_assign);

            while (
                static_cast<int>(find_parse_rule(token_iter_->type).precedence) >=
                static_cast<int>(min_precedence)
            ) {
                const auto infix_rule = find_parse_rule(token_iter_->type).infix;
                (this->*infix_rule)(can_assign);
            }

            if (can_assign && token_iter_->type == Token_type::equal) {
                throw Compiler_error{*token_iter_, "Invalid assignment target."};
            }
        }

        void compile_declaration() {
            try {
                if (token_iter_->type == Token_type::var) {
                    compile_var_declaration();
                } else {
                    compile_statement();
                }
            } catch (const Compiler_error& error) {
                on_resumable_error_(error);
                recover_to_synchronization_point();
            }
        }

        void compile_statement() {
            if (token_iter_->type == Token_type::print) {
                compile_print_statement();
            } else {
                compile_expression_statement();
            }
        }

        void compile_print_statement() {
            const auto line = token_iter_->line;
            ++token_iter_;

            compile_expression();
            consume(Token_type::semicolon, "Expected ';' after value.");
            chunk_.bytecode_push_back(Op_code::print, line);
        }

        void compile_expression() {
            compile_precedence_or_higher(Precedence::assignment);
        }

        void compile_var_declaration() {
            const auto line = token_iter_->line;
            ++token_iter_;

            const auto identifier_offset = chunk_.constants_push_back(Value{
                string{token_iter_->begin, token_iter_->end}
            });
            consume(Token_type::identifier, "Expected variable name.");

            if (consume_if_match(Token_type::equal)) {
                compile_expression();
            } else {
                chunk_.bytecode_push_back(Op_code::nil, line);
            }
            consume(Token_type::semicolon, "Expected ';' after variable declaration.");

            chunk_.bytecode_push_back(Op_code::define_global, line);
            chunk_.bytecode_push_back(identifier_offset, line);
        }

        void compile_expression_statement() {
            compile_expression();
            const auto line = token_iter_->line;
            consume(Token_type::semicolon, "Expected ';' after expression.");
            chunk_.bytecode_push_back(Op_code::pop, line);
        }

        void compile_grouping(bool /*can_assign*/) {
            ++token_iter_;
            compile_expression();
            consume(Token_type::right_paren, "Expected ')' after expression.");
        }

        void compile_number(bool /*can_assign*/) {
            const auto value = lexical_cast<double>(string{token_iter_->begin, token_iter_->end});
            const auto offset = chunk_.constants_push_back(Value{value});

            chunk_.bytecode_push_back(Op_code::constant, token_iter_->line);
            chunk_.bytecode_push_back(offset, token_iter_->line);

            ++token_iter_;
        }

        void compile_string(bool /*can_assign*/) {
            // The +1 and -1 parts trim the leading and trailing quotation marks
            const auto offset = chunk_.constants_push_back(Value{string{token_iter_->begin + 1, token_iter_->end - 1}});

            chunk_.bytecode_push_back(Op_code::constant, token_iter_->line);
            chunk_.bytecode_push_back(offset, token_iter_->line);

            ++token_iter_;
        }

        void compile_variable(bool can_assign) {
            const auto identifier_offset = chunk_.constants_push_back(Value{
                string{token_iter_->begin, token_iter_->end}
            });

            const auto line = token_iter_->line;
            ++token_iter_;

            if (can_assign && consume_if_match(Token_type::equal)) {
                compile_expression();
                chunk_.bytecode_push_back(Op_code::set_global, line);
            } else {
                chunk_.bytecode_push_back(Op_code::get_global, line);
            }
            chunk_.bytecode_push_back(identifier_offset, line);
        }

        void compile_unary(bool /*can_assign*/) {
            const auto op = *move(token_iter_);
            ++token_iter_;

            // Compile the operand
            compile_precedence_or_higher(Precedence::unary);

            switch (op.type) {
                case Token_type::bang:
                    chunk_.bytecode_push_back(Op_code::not_, op.line);
                    break;

                case Token_type::minus:
                    chunk_.bytecode_push_back(Op_code::negate, op.line);
                    break;

                default:
                    throw Compiler_error{op, "Unreachable."};
            }
        }

        void compile_binary(bool /*can_assign*/) {
            const auto op = *move(token_iter_);
            ++token_iter_;

            // Compile the right operand
            const auto rule = find_parse_rule(op.type);
            const auto higher_level_precedence = static_cast<Precedence>(static_cast<int>(rule.precedence) + 1);
            compile_precedence_or_higher(higher_level_precedence);

            switch (op.type) {
                case Token_type::bang_equal:
                    chunk_.bytecode_push_back(Op_code::equal, op.line);
                    chunk_.bytecode_push_back(Op_code::not_, op.line);
                    break;

                case Token_type::equal_equal:
                    chunk_.bytecode_push_back(Op_code::equal, op.line);
                    break;

                case Token_type::greater:
                    chunk_.bytecode_push_back(Op_code::greater, op.line);
                    break;

                case Token_type::greater_equal:
                    chunk_.bytecode_push_back(Op_code::less, op.line);
                    chunk_.bytecode_push_back(Op_code::not_, op.line);
                    break;

                case Token_type::less:
                    chunk_.bytecode_push_back(Op_code::less, op.line);
                    break;

                case Token_type::less_equal:
                    chunk_.bytecode_push_back(Op_code::greater, op.line);
                    chunk_.bytecode_push_back(Op_code::not_, op.line);
                    break;

                case Token_type::plus:
                    chunk_.bytecode_push_back(Op_code::add, op.line);
                    break;

                case Token_type::minus:
                    chunk_.bytecode_push_back(Op_code::subtract, op.line);
                    break;

                case Token_type::star:
                    chunk_.bytecode_push_back(Op_code::multiply, op.line);
                    break;

                case Token_type::slash:
                    chunk_.bytecode_push_back(Op_code::divide, op.line);
                    break;

                default:
                    throw Compiler_error{op, "Unreachable."};
            }
        }

        void compile_literal(bool /*can_assign*/) {
            switch (token_iter_->type) {
                case Token_type::false_:
                    chunk_.bytecode_push_back(Op_code::false_, token_iter_->line);
                    break;

                case Token_type::nil:
                    chunk_.bytecode_push_back(Op_code::nil, token_iter_->line);
                    break;

                case Token_type::true_:
                    chunk_.bytecode_push_back(Op_code::true_, token_iter_->line);
                    break;

                default:
                    throw Compiler_error{*token_iter_, "Unreachable."};
            }

            ++token_iter_;
        }

        void consume(Token_type type, const string& message) {
            if (token_iter_->type != type) {
                throw Compiler_error{*token_iter_, message};
            }

            ++token_iter_;
        }

        bool consume_if_match(Token_type token_type) {
            if (token_iter_->type == token_type) {
                ++token_iter_;
                return true;
            }

            return false;
        }

        void recover_to_synchronization_point() {
            while (token_iter_->type != Token_type::eof) {
                if (consume_if_match(Token_type::semicolon)) {
                    return;
                }

                switch (token_iter_->type) {
                    case Token_type::class_:
                    case Token_type::fun:
                    case Token_type::var:
                    case Token_type::for_:
                    case Token_type::if_:
                    case Token_type::while_:
                    case Token_type::print:
                    case Token_type::return_:
                        return;

                    default:
                        ++token_iter_;
                }
            }
        }
    };
}

namespace motts { namespace lox {
    Chunk compile(const string& source) {
        string compiler_errors;
        Compiler compiler {
            source,
            [&] (const Compiler_error& error) {
                compiler_errors += error.what();
                compiler_errors += "\n";
            }
        };
        while (compiler.token_iter_->type != Token_type::eof) {
            compiler.compile_declaration();
        }
        compiler.consume(Token_type::eof, "Expected end of expression.");

        if (!compiler_errors.empty()) {
            throw Compiler_error{compiler_errors};
        }

        disassemble_chunk(compiler.chunk_, "code");
        compiler.chunk_.bytecode_push_back(Op_code::return_, 0);

        return move(compiler.chunk_);
    }

    Compiler_error::Compiler_error(const Token& token, const string& what) :
        runtime_error{
            "[Line " + to_string(token.line) + "] Error" +
            (
                token.type == Token_type::eof ?
                    " at end" :
                    " at '" + string{token.begin, token.end} + "'"
            ) +
            ": " + what
        }
    {}
}}
