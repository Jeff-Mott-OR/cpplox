#include "compiler.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <gsl/gsl_util>

#include "debug.hpp"

using std::find_if;
using std::function;
using std::move;
using std::runtime_error;
using std::string;
using std::to_string;
using std::uint16_t;
using std::vector;

using boost::lexical_cast;
using gsl::finally;
using gsl::narrow;

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
            { nullptr,                     &Compiler::compile_and,     Precedence::and_ },   // AND
            { nullptr,                     nullptr,                    Precedence::none },   // CLASS
            { nullptr,                     nullptr,                    Precedence::none },   // ELSE
            { &Compiler::compile_literal,  nullptr,                    Precedence::none },   // FALSE
            { nullptr,                     nullptr,                    Precedence::none },   // FOR
            { nullptr,                     nullptr,                    Precedence::none },   // FUN
            { nullptr,                     nullptr,                    Precedence::none },   // IF
            { &Compiler::compile_literal,  nullptr,                    Precedence::none },   // NIL
            { nullptr,                     &Compiler::compile_or,      Precedence::or_ },   // OR
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

        struct Local {
            Token name;
            int n_stack_frame {};
        };
        vector<Local> locals_;
        int n_stack_frames_ {};

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

        void pop_stack_frame() {
            while (!locals_.empty() && locals_.back().n_stack_frame == n_stack_frames_) {
                chunk_.bytecode_push_back(Op_code::pop, token_iter_->line);
                locals_.pop_back();
            }
            --n_stack_frames_;
        }

        auto emit_jump(Op_code jump_kind) {
            const auto jump_instruction_offset = chunk_.code.size();

            chunk_.bytecode_push_back(jump_kind, token_iter_->line);
            chunk_.bytecode_push_back(0xff, token_iter_->line);
            chunk_.bytecode_push_back(0xff, token_iter_->line);

            return jump_instruction_offset;
        }

        void patch_jump(int jump_instruction_offset) {
            patch_jump(jump_instruction_offset, chunk_.code.size() - jump_instruction_offset - 3);
        }

        void patch_jump(int jump_instruction_offset, int jump_length) {
            // DANGER! Reinterpret cast: After a jump instruction, there will be two adjacent bytes
            // that are supposed to represent a single uint16 number
            reinterpret_cast<uint16_t&>(chunk_.code.at(jump_instruction_offset + 1)) = narrow<uint16_t>(jump_length);
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
            } else if (token_iter_->type == Token_type::for_) {
                compile_for_statement();
            } else if (token_iter_->type == Token_type::if_) {
                compile_if_statement();
            } else if (token_iter_->type == Token_type::while_) {
                compile_while_statement();
            } else if (token_iter_->type == Token_type::left_brace) {
                ++n_stack_frames_;
                const auto _ = finally([&] () {
                    pop_stack_frame();
                });

                compile_block();
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

        void compile_while_statement() {
            const auto line = token_iter_->line;
            ++token_iter_;

            const auto loop_start_offset = chunk_.code.size();
            consume(Token_type::left_paren, "Expected '(' after 'while'.");
            compile_expression();
            consume(Token_type::right_paren, "Expected ')' after condition.");
            const auto exit_placeholder_offset = emit_jump(Op_code::jump_if_false);

            chunk_.bytecode_push_back(Op_code::pop, line);
            compile_statement();
            const auto jump_length = chunk_.code.size() - loop_start_offset;
            const auto loop_placeholder_offset = emit_jump(Op_code::loop);
            patch_jump(loop_placeholder_offset, jump_length);

            patch_jump(exit_placeholder_offset);
            chunk_.bytecode_push_back(Op_code::pop, line);
        }

        void compile_expression() {
            compile_precedence_or_higher(Precedence::assignment);
        }

        void compile_block() {
            ++token_iter_;
            while (token_iter_->type != Token_type::right_brace && token_iter_->type != Token_type::eof) {
                compile_declaration();
            }
            consume(Token_type::right_brace, "Expected '}' after block.");
        }

        void compile_var_declaration() {
            const auto line = token_iter_->line;
            ++token_iter_;

            const auto var_name = string{token_iter_->begin, token_iter_->end};

            // Stack frame 0 means global
            if (n_stack_frames_ != 0) {
                // It's an error to have two variables with the same name in the same local scope
                const auto found_same_name = find_if(locals_.cbegin(), locals_.cend(), [&] (const auto& local) {
                    return local.n_stack_frame == n_stack_frames_ && string{local.name.begin, local.name.end} == var_name;
                });
                if (found_same_name != locals_.cend()) {
                    throw Compiler_error{*token_iter_, "Variable with this name already declared in this scope."};
                }

                locals_.push_back({*token_iter_, -1});
            }

            consume(Token_type::identifier, "Expected variable name.");

            if (consume_if_match(Token_type::equal)) {
                compile_expression();
            } else {
                chunk_.bytecode_push_back(Op_code::nil, line);
            }
            consume(Token_type::semicolon, "Expected ';' after variable declaration.");

            if (n_stack_frames_ != 0) {
                locals_.back().n_stack_frame = n_stack_frames_;
            } else {
                const auto identifier_offset = chunk_.constants_push_back(Value{var_name});
                chunk_.bytecode_push_back(Op_code::define_global, line);
                chunk_.bytecode_push_back(identifier_offset, line);
            }
        }

        void compile_and(bool /*can_assign*/) {
            const auto exit_placeholder_offset = emit_jump(Op_code::jump_if_false);
            chunk_.bytecode_push_back(Op_code::pop, token_iter_->line);
            compile_precedence_or_higher(Precedence::and_);
            patch_jump(exit_placeholder_offset);
        }

        void compile_or(bool /*can_assign*/) {
            const auto else_placeholder_offset = emit_jump(Op_code::jump_if_false);
            const auto exit_placeholder_offset = emit_jump(Op_code::jump);

            patch_jump(else_placeholder_offset);
            chunk_.bytecode_push_back(Op_code::pop, token_iter_->line);
            compile_precedence_or_higher(Precedence::or_);

            patch_jump(exit_placeholder_offset);
        }

        void compile_expression_statement() {
            compile_expression();
            const auto line = token_iter_->line;
            consume(Token_type::semicolon, "Expected ';' after expression.");
            chunk_.bytecode_push_back(Op_code::pop, line);
        }

        void compile_for_statement() {
            ++token_iter_;

            // If a for statement declares a variable, that variable should be scoped to the loop body
            ++n_stack_frames_;
            const auto _ = finally([&] () {
                pop_stack_frame();
            });

            consume(Token_type::left_paren, "Expected '(' after 'for'.");

            if (consume_if_match(Token_type::semicolon)) {
                // No initializer
            } else if (token_iter_->type == Token_type::var) {
                compile_var_declaration();
            } else {
                compile_expression_statement();
            }

            const auto condition_expression_offset = chunk_.code.size();
            const auto empty_offset_sentinel = -1;

            const auto jump_to_exit_offset = ([&] () {
                if (consume_if_match(Token_type::semicolon)) {
                    return empty_offset_sentinel;
                }

                compile_expression();
                const auto jump_to_exit_offset = narrow<int>(emit_jump(Op_code::jump_if_false));
                chunk_.bytecode_push_back(Op_code::pop, token_iter_->line);
                consume(Token_type::semicolon, "Expected ';' after loop condition.");

                return jump_to_exit_offset;
            })();

            const auto increment_expression_offset = ([&] () {
                if (consume_if_match(Token_type::right_paren)) {
                    return empty_offset_sentinel;
                }

                const auto jump_to_body_offset = emit_jump(Op_code::jump);

                const auto increment_expression_offset = narrow<int>(chunk_.code.size());
                compile_expression();
                chunk_.bytecode_push_back(Op_code::pop, token_iter_->line);
                consume(Token_type::right_paren, "Expected ')' after for clauses.");

                const auto loop_to_condition_length = chunk_.code.size() - condition_expression_offset;
                const auto loop_to_condition_offset = emit_jump(Op_code::loop);
                patch_jump(loop_to_condition_offset, loop_to_condition_length);

                patch_jump(jump_to_body_offset);

                return increment_expression_offset;
            })();

            const auto line = token_iter_->line;
            compile_statement();

            const auto loop_length = chunk_.code.size() - (
                increment_expression_offset != empty_offset_sentinel ?
                    increment_expression_offset :
                    condition_expression_offset
            );
            const auto loop_to_increment_offset = emit_jump(Op_code::loop);
            patch_jump(loop_to_increment_offset, loop_length);

            if (jump_to_exit_offset != empty_offset_sentinel) {
                patch_jump(jump_to_exit_offset);
                chunk_.bytecode_push_back(Op_code::pop, line);
            }
        }

        void compile_if_statement() {
            const auto line = token_iter_->line;
            ++token_iter_;

            consume(Token_type::left_paren, "Expected '(' after 'if'.");
            compile_expression();
            consume(Token_type::right_paren, "Expected ')' after 'condition'.");
            const auto else_placeholder_offset = emit_jump(Op_code::jump_if_false);

            chunk_.bytecode_push_back(Op_code::pop, line);
            compile_statement();
            const auto exit_placeholder_offset = emit_jump(Op_code::jump);

            patch_jump(else_placeholder_offset);
            chunk_.bytecode_push_back(Op_code::pop, line);
            if (consume_if_match(Token_type::else_)) {
                compile_statement();
            }
            patch_jump(exit_placeholder_offset);
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
            const auto var_name = string{token_iter_->begin, token_iter_->end};
            const auto found_local = find_if(locals_.crbegin(), locals_.crend(), [&] (const auto& local) {
                return string{local.name.begin, local.name.end} == var_name;
            });

            if (found_local != locals_.crend() && found_local->n_stack_frame == -1) {
                throw Compiler_error{*token_iter_, "Cannot read local variable in its own initializer."};
            }

            const auto line = token_iter_->line;
            ++token_iter_;

            if (can_assign && consume_if_match(Token_type::equal)) {
                compile_expression();

                if (found_local != locals_.crend()) {
                    chunk_.bytecode_push_back(Op_code::set_local, line);
                } else {
                    chunk_.bytecode_push_back(Op_code::set_global, line);
                }
            } else {
                if (found_local != locals_.crend()) {
                    chunk_.bytecode_push_back(Op_code::get_local, line);
                } else {
                    chunk_.bytecode_push_back(Op_code::get_global, line);
                }
            }

            if (found_local != locals_.crend()) {
                chunk_.bytecode_push_back(found_local.base() - locals_.crend().base() - 1, line);
            } else {
                chunk_.bytecode_push_back(chunk_.constants_push_back(Value{var_name}), line);
            }
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
        compiler.chunk_.bytecode_push_back(Op_code::return_, 0);

        if (!compiler_errors.empty()) {
            throw Compiler_error{compiler_errors};
        }

        disassemble_chunk(compiler.chunk_, "code");

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
