#include "compiler.hpp"

#include <cstdint>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

#include "scanner.hpp"

// Not exported (internal linkage)
namespace {
    // Allow the internal linkage section to access names
    using namespace motts::lox;

    struct Compiler_error : std::runtime_error {
        Compiler_error(const Token& debug_token, const char* message) :
            std::runtime_error{
                "[Line " + std::to_string(debug_token.line) + "] "
                "Error at " + (
                    debug_token.type != Token_type::eof ? ('\'' + gsl::to_string(debug_token.lexeme) + '\'') : "end"
                ) + ": " + message
            }
        {}
    };

    struct Resumable_compiler_error : Compiler_error {
        using Compiler_error::Compiler_error;
    };

    struct Tracked_local {
        Token name;
        int depth {0};
        bool initialized {true};
        bool captured {false};
    };

    auto operator==(const Tracked_local& lhs, const Tracked_local& rhs) {
        return lhs.name.lexeme == rhs.name.lexeme && lhs.depth == rhs.depth;
    }

    struct Tracked_upvalue {
        bool is_direct_capture {};
        std::uint8_t enclosing_index {};
    };

    auto operator==(const Tracked_upvalue& lhs, const Tracked_upvalue& rhs) {
        return lhs.is_direct_capture == rhs.is_direct_capture &&
            lhs.enclosing_index == rhs.enclosing_index;
    }

    struct Tracked_class {
        bool has_superclass {false};
    };

    struct Tracked_loop {
        std::size_t loop_begin_offset;
        std::vector<std::function<void()>> on_loop_emit_end;

        Tracked_loop(std::size_t loop_begin_offset_arg) :
            loop_begin_offset{loop_begin_offset_arg}
        {
        }
    };

    enum class Function_type {
        function,
        initializer,
        method,
        script
    };

    struct Function_translation_unit {
        GC_ptr<Function> function;
        Function_type fn_type;
        int scope_depth {};
        std::vector<Tracked_local> tracked_locals;
        std::vector<Tracked_upvalue> tracked_upvalues;

        Function_translation_unit(const GC_ptr<Function>& function_arg, Function_type fn_type_arg) :
            function {function_arg},
            fn_type {fn_type_arg}
        {}

        auto emit(std::uint8_t byte, const Token& debug_token) {
            function->chunk.opcodes.push_back(byte);
            function->chunk.tokens.push_back(debug_token);
        }

        auto emit(Opcode byte, const Token& debug_token) {
            emit(gsl::narrow<std::uint8_t>(byte), debug_token);
        }

        template<typename Byte_T, typename Byte_U>
            void emit(Byte_T first, Byte_U second, const Token& debug_token) {
                emit(first, debug_token);
                emit(second, debug_token);
            }

        auto begin_scope() {
            ++scope_depth;
        }

        auto end_scope(const Token& debug_token) {
            auto local_iter = tracked_locals.crbegin();
            for ( ; local_iter != tracked_locals.crend() && local_iter->depth >= scope_depth; ++local_iter) {
                emit(local_iter->captured ? Opcode::close_upvalue : Opcode::pop, debug_token);
            }
            tracked_locals.erase(local_iter.base(), tracked_locals.cend());

            --scope_depth;
        }

        void track_local(const Token& local_name, bool initialized = true) {
            if (tracked_locals.size() >= std::numeric_limits<std::uint8_t>::max()) {
                throw Resumable_compiler_error{local_name, "Too many local variables in function."};
            }

            Tracked_local new_local {local_name, scope_depth, initialized};

            const auto found_redeclared = std::find(tracked_locals.cbegin(), tracked_locals.cend(), new_local);
            if (found_redeclared != tracked_locals.cend()) {
                throw Compiler_error{local_name, "Variable with this name already declared in this scope."};
            }

            tracked_locals.push_back(std::move(new_local));
        }

        auto track_upvalue(Tracked_upvalue&& upvalue, const Token& local_token) {
            const auto found_upvalue_iter = std::find(tracked_upvalues.cbegin(), tracked_upvalues.cend(), upvalue);
            if (found_upvalue_iter != tracked_upvalues.cend()) {
                return gsl::narrow<std::uint8_t>(std::distance(tracked_upvalues.cbegin(), found_upvalue_iter));
            }

            if (tracked_upvalues.size() >= std::numeric_limits<std::uint8_t>::max()) {
                throw Resumable_compiler_error{local_token, "Too many closure variables in function."};
            }

            tracked_upvalues.push_back(std::move(upvalue));

            return gsl::narrow<std::uint8_t>(tracked_upvalues.size() - 1);
        }

        auto find_local(const gsl::cstring_span<>& name) {
            const auto found_local_iter = std::find_if(tracked_locals.rbegin(), tracked_locals.rend(), [&] (const auto& tracked_local) {
                return tracked_local.name.lexeme == name;
            });
            if (found_local_iter == tracked_locals.rend()) {
                return tracked_locals.end();
            }

            return found_local_iter.base() - 1;
        }

        auto local_index(std::vector<Tracked_local>::const_iterator local_iter) const {
            return gsl::narrow<std::uint8_t>(std::distance(tracked_locals.cbegin(), local_iter));
        }

        auto local_index(const gsl::cstring_span<>& name) {
            const auto found_local_iter = find_local(name);
            assert(found_local_iter != tracked_locals.cend() && "Local not found");
            return local_index(found_local_iter);
        }
    };

    // Order matters; the order is by precedence level, from lowest to highest
    enum class Precedence_level {
        none,
        assignment,  // =
        or_,          // or
        and_,         // and
        equality,    // == !=
        comparison,  // < > <= >=
        term,        // + -
        factor,      // * /
        unary,       // ! -
        call,        // . ()
        primary
    };

    // There's no invariant being maintained here; this exists primarily to avoid lots of manual argument passing
    struct Compiler {
        GC_heap& gc_heap;
        Interned_strings& interned_strings;
        Token_iterator token_iter;
        std::vector<Function_translation_unit> translation_units;
        std::vector<Tracked_class> tracked_classes;
        std::vector<Tracked_loop> tracked_loops;
        std::function<void(const std::exception&)> on_resumable_error;

        Compiler(
            GC_heap& gc_heap_arg,
            Interned_strings& interned_strings_arg,
            Token_iterator&& token_iter_arg,
            std::function<void(const std::exception&)>&& on_resumable_error_arg
        ) :
            gc_heap {gc_heap_arg},
            interned_strings {interned_strings_arg},
            token_iter {std::move(token_iter_arg)},
            on_resumable_error {std::move(on_resumable_error_arg)}
        {
            translation_units.push_back({gc_heap.make(Function{interned_strings.get("__script__")}), Function_type::script});
            translation_units.back().track_local(Token{Token_type::identifier, "__script__"});
        }

        void emit_getter(const Token& name) {
            const auto local_iter = translation_units.back().find_local(name.lexeme);
            if (local_iter != translation_units.back().tracked_locals.end()) {
                if (!local_iter->initialized) {
                    throw Resumable_compiler_error{name, "Cannot read local variable in its own initializer."};
                }

                translation_units.back().emit(Opcode::get_local, translation_units.back().local_index(local_iter), name);

                return;
            }

            const auto maybe_upvalue_index = track_upvalue_chain(name);
            if (maybe_upvalue_index) {
                translation_units.back().emit(Opcode::get_upvalue, *maybe_upvalue_index, name);

                return;
            }

            translation_units.back().function->chunk.constants.push_back({interned_strings.get(name.lexeme)});
            translation_units.back().emit(Opcode::get_global, translation_units.back().function->chunk.constants.size() - 1, name);
        }

        void emit_setter(const Token& name) {
            const auto local_iter = translation_units.back().find_local(name.lexeme);
            if (local_iter != translation_units.back().tracked_locals.end()) {
                if (!local_iter->initialized) {
                    throw Resumable_compiler_error{name, "Cannot read local variable in its own initializer."};
                }

                translation_units.back().emit(Opcode::set_local, translation_units.back().local_index(local_iter), name);

                return;
            }

            const auto maybe_upvalue_index = track_upvalue_chain(name);
            if (maybe_upvalue_index) {
                translation_units.back().emit(Opcode::set_upvalue, *maybe_upvalue_index, name);

                return;
            }

            translation_units.back().function->chunk.constants.push_back({interned_strings.get(name.lexeme)});
            translation_units.back().emit(Opcode::set_global, translation_units.back().function->chunk.constants.size() - 1, name);
        }

        std::optional<std::uint8_t> track_upvalue_chain(std::vector<Function_translation_unit>::iterator tu, const Token& local_token) {
            // If we've recursed to the root, then we didn't find an upvalue; presumed global
            if (tu == translation_units.begin()) {
                return {};
            }

            const auto enclosing_tu = tu - 1;
            const auto enclosing_local_iter = enclosing_tu->find_local(local_token.lexeme);
            if (enclosing_local_iter != enclosing_tu->tracked_locals.end()) {
                enclosing_local_iter->captured = true;
                return tu->track_upvalue({true, enclosing_tu->local_index(enclosing_local_iter)}, local_token);
            }

            const auto enclosing_upvalue_index = track_upvalue_chain(enclosing_tu, local_token);
            if (enclosing_upvalue_index) {
                return tu->track_upvalue({false, *enclosing_upvalue_index}, local_token);
            }

            // Presumed global
            return {};
        }

        std::optional<std::uint8_t> track_upvalue_chain(const Token& local_token) {
            return track_upvalue_chain(translation_units.end() - 1, local_token);
        }

        Token consume(Token_type type, const char* error_message) {
            if (token_iter->type != type) {
                throw Resumable_compiler_error{*token_iter, error_message};
            }

            return *token_iter++;
        }

        bool consume_if(Token_type type) {
            if (token_iter->type != type) {
                return false;
            }

            ++token_iter;
            return true;
        }

        void compile_declaration() {
          try {
              if (consume_if(Token_type::var_)) {
                  compile_var_declaration();
              } else if (consume_if(Token_type::fun_)) {
                  compile_function_declaration();
              } else if (consume_if(Token_type::class_)) {
                  compile_class_declaration();
              } else {
                  compile_statement();
              }
          } catch (const Resumable_compiler_error& error) {
              on_resumable_error(error);
              synchronize();
          }
        }

        void synchronize() {
            while (token_iter->type != Token_type::eof) {
                if (consume_if(Token_type::semicolon)) {
                    return;
                }

                switch (token_iter->type) {
                    case Token_type::var_:
                    case Token_type::fun_:
                    case Token_type::class_:
                    case Token_type::print:
                    case Token_type::return_:
                    case Token_type::if_:
                    case Token_type::for_:
                    case Token_type::while_:
                        return;

                    default:
                        ++token_iter;
                }
            }
        }

        void compile_var_declaration() {
            const auto var_name_token = consume(Token_type::identifier, "Expected variable name.");
            if (translation_units.size() > 1 || translation_units.back().scope_depth > 0) {
                translation_units.back().track_local(var_name_token, false);
            }

            if (consume_if(Token_type::equal)) {
                compile_expr_higher_precedence_than(Precedence_level::assignment);
            } else {
                translation_units.back().emit(Opcode::nil, var_name_token);
            }

            consume(Token_type::semicolon, "Expected ';' after variable declaration.");

            if (translation_units.size() > 1 || translation_units.back().scope_depth > 0) {
                translation_units.back().tracked_locals.back().initialized = true;
            } else {
                translation_units.back().function->chunk.constants.push_back({interned_strings.get(var_name_token.lexeme)});
                translation_units.back().emit(Opcode::define_global, translation_units.back().function->chunk.constants.size() - 1, var_name_token);
            }
        }

        void compile_function_declaration() {
            const auto fn_name_token = consume(Token_type::identifier, "Expected function name.");
            if (translation_units.size() > 1 || translation_units.back().scope_depth > 0) {
                translation_units.back().track_local(fn_name_token);
            }

            compile_function_rest(fn_name_token, Function_type::function);

            if (translation_units.size() == 1 && translation_units.back().scope_depth == 0) {
                translation_units.back().function->chunk.constants.push_back({interned_strings.get(fn_name_token.lexeme)});
                translation_units.back().emit(Opcode::define_global, translation_units.back().function->chunk.constants.size() - 1, fn_name_token);
            }
        }

        void compile_function_rest(const Token& fn_name_token, Function_type fn_type) {
            translation_units.push_back({gc_heap.make(Function{interned_strings.get(fn_name_token.lexeme)}), fn_type});
            const auto _ = gsl::finally([&] () {
                translation_units.pop_back();
            });

            if (fn_type == Function_type::initializer || fn_type == Function_type::method) {
                translation_units.back().track_local({Token_type::this_, "this"});
            } else {
                translation_units.back().track_local(fn_name_token);
            }

            // Parameter list
            consume(Token_type::left_paren, "Expected '(' after function name.");
            if (token_iter->type != Token_type::right_paren) {
                do {
                    const auto param_name_token = consume(Token_type::identifier, "Expected parameter name.");
                    translation_units.back().track_local(param_name_token);
                    ++translation_units.back().function->arity;
                } while (consume_if(Token_type::comma));
            }
            if (translation_units.back().function->arity > 8) {
                throw Resumable_compiler_error{*token_iter, "Cannot have more than 8 parameters."};
            }
            consume(Token_type::right_paren, "Expected ')' after parameters.");

            // Body
            consume(Token_type::left_brace, "Expected '{' before function body.");
            while (token_iter->type != Token_type::eof && token_iter->type != Token_type::right_brace) {
                compile_declaration();
            }
            consume(Token_type::right_brace, "Expected '}' after block.");

            // End of function will implicitly return either nil or this
            if (fn_type == Function_type::initializer) {
                translation_units.back().emit(Opcode::get_local, translation_units.back().local_index("this"), fn_name_token);
            } else {
                translation_units.back().emit(Opcode::nil, *token_iter);
            }
            translation_units.back().emit(Opcode::return_, *token_iter);

            // Add to enclosing constants
            const auto enclosing_tu = translation_units.end() - 2;
            if (enclosing_tu->function->chunk.constants.size() >= std::numeric_limits<std::uint8_t>::max()) {
                throw Resumable_compiler_error{fn_name_token, "Too many constants in one chunk."};
            }
            enclosing_tu->function->chunk.constants.push_back({translation_units.back().function});
            enclosing_tu->emit(Opcode::closure, enclosing_tu->function->chunk.constants.size() - 1, fn_name_token);

            for (const auto& tracked_upvalue : translation_units.back().tracked_upvalues) {
                enclosing_tu->emit(tracked_upvalue.is_direct_capture ? 1 : 0, tracked_upvalue.enclosing_index, fn_name_token);
            }
            translation_units.back().function->upvalue_count = translation_units.back().tracked_upvalues.size();
        }

        void compile_class_declaration() {
            // Track when we're in a class and when we're not, so we can validate uses of "this" and "super"
            tracked_classes.push_back({});
            const auto _ = gsl::finally([&] () {
                tracked_classes.pop_back();
            });

            const auto class_name_token = consume(Token_type::identifier, "Expected class name.");
            translation_units.back().function->chunk.constants.push_back({interned_strings.get(class_name_token.lexeme)});
            const auto class_name_constant_index = translation_units.back().function->chunk.constants.size() - 1;
            translation_units.back().emit(Opcode::class_, class_name_constant_index, class_name_token);

            if (translation_units.size() > 1 || translation_units.back().scope_depth > 0) {
                translation_units.back().track_local(class_name_token);
            } else {
                translation_units.back().emit(Opcode::define_global, class_name_constant_index, class_name_token);
            }

            std::optional<gsl::final_act<std::function<void()>>> finally_end_superclass_scope;
            if (consume_if(Token_type::less)) {
                tracked_classes.back().has_superclass = true;

                const auto superclass_name_token = consume(Token_type::identifier, "Expected superclass name.");
                if (superclass_name_token.lexeme == class_name_token.lexeme) {
                    throw Resumable_compiler_error{superclass_name_token, "A class cannot inherit from itself."};
                }

                emit_getter(superclass_name_token);
                emit_getter(class_name_token);
                translation_units.back().emit(Opcode::inherit, superclass_name_token);

                // Implicitly local "super"
                translation_units.back().begin_scope();
                finally_end_superclass_scope = gsl::finally(std::function<void()>{[&] () {
                    translation_units.back().end_scope(*token_iter);
                }});
                translation_units.back().track_local({Token_type::super_, "super", superclass_name_token.line});
            }

            emit_getter(class_name_token);

            consume(Token_type::left_brace, "Expected '{' before class body.");
            while (token_iter->type != Token_type::eof && token_iter->type != Token_type::right_brace) {
                const auto method_name_token = consume(Token_type::identifier, "Expected method name.");
                translation_units.back().function->chunk.constants.push_back({interned_strings.get(method_name_token.lexeme)});
                const auto method_name_constant_index = translation_units.back().function->chunk.constants.size() - 1;

                compile_function_rest(method_name_token, method_name_token.lexeme == "init" ? Function_type::initializer : Function_type::method);

                translation_units.back().emit(Opcode::method, method_name_constant_index, method_name_token);
            }
            const auto right_brace_token = consume(Token_type::right_brace, "Expected '}' after class body.");

            translation_units.back().emit(Opcode::pop, right_brace_token);
        }

        void compile_statement() {
            const auto stmt_begin_token = *token_iter;

            if (consume_if(Token_type::print)) {
                compile_expr_higher_precedence_than(Precedence_level::assignment);
                consume(Token_type::semicolon, "Expected ';' after value.");
                translation_units.back().emit(Opcode::print, stmt_begin_token);
            } else if (consume_if(Token_type::return_)) {
                compile_return_statement(stmt_begin_token);
            } else if (consume_if(Token_type::if_)) {
                compile_if_statement();
            } else if (consume_if(Token_type::for_)) {
                compile_for_statement();
            } else if (consume_if(Token_type::while_)) {
                compile_while_statement();
            } else if (consume_if(Token_type::break_)) {
                compile_break_statement(stmt_begin_token);
            } else if (consume_if(Token_type::continue_)) {
                compile_continue_statement(stmt_begin_token);
            } else if (consume_if(Token_type::left_brace)) {
                translation_units.back().begin_scope();
                const auto _ = gsl::finally([&] () {
                    translation_units.back().end_scope(*token_iter);
                });

                while (token_iter->type != Token_type::eof && token_iter->type != Token_type::right_brace) {
                    compile_declaration();
                }
                consume(Token_type::right_brace, "Expected '}' after block.");
            } else {
                compile_expression_statement();
           }
        }

        void compile_for_statement() {
            translation_units.back().begin_scope();
            const auto _ = gsl::finally([&] () {
                translation_units.back().end_scope(*token_iter);
            });

            // Initializer
            consume(Token_type::left_paren, "Expected '(' after 'for'.");
            if (consume_if(Token_type::semicolon)) {
                // No initializer
            } else if (consume_if(Token_type::var_)) {
                compile_var_declaration();
            } else {
                compile_expression_statement();
            }

            // Condition
            const auto condition_begin_offset = translation_units.back().function->chunk.opcodes.size();
            std::optional<std::size_t> condition_false_jump_distance_offset;
            const auto condition_expr_begin_token = *token_iter;
            if (!consume_if(Token_type::semicolon)) {
                compile_expr_higher_precedence_than(Precedence_level::assignment);
                consume(Token_type::semicolon, "Expected ';' after loop condition.");

                translation_units.back().emit(Opcode::jump_if_false, condition_expr_begin_token);
                condition_false_jump_distance_offset = translation_units.back().function->chunk.opcodes.size();
                translation_units.back().emit(0, 0, condition_expr_begin_token);

                // But if condition is true, pop expr result and fall through
                translation_units.back().emit(Opcode::pop, condition_expr_begin_token);
            }

            // Increment
            std::optional<std::size_t> increment_begin_offset;
            if (!consume_if(Token_type::right_paren)) {
                const auto increment_expr_begin_token = *token_iter;

                // After a truthy condition, jump past the increment
                translation_units.back().emit(Opcode::jump, increment_expr_begin_token);
                const auto increment_begin_to_body_begin_jump_distance_offset = translation_units.back().function->chunk.opcodes.size();
                translation_units.back().emit(0, 0, increment_expr_begin_token);

                // Compile increment
                increment_begin_offset = translation_units.back().function->chunk.opcodes.size();
                compile_expr_higher_precedence_than(Precedence_level::assignment);
                translation_units.back().emit(Opcode::pop, increment_expr_begin_token);
                consume(Token_type::right_paren, "Expected ')' after for clauses.");

                // After increment, jump back to condition
                translation_units.back().emit(Opcode::loop, increment_expr_begin_token);
                const auto increment_end_to_condition_begin_jump_distance = translation_units.back().function->chunk.opcodes.size() - condition_begin_offset + 2;
                if (increment_end_to_condition_begin_jump_distance > std::numeric_limits<std::uint16_t>::max()) {
                    throw Resumable_compiler_error{increment_expr_begin_token, "Loop body too large."};
                }
                translation_units.back().emit(increment_end_to_condition_begin_jump_distance >> 8, increment_end_to_condition_begin_jump_distance & 0xff, increment_expr_begin_token);

                // Patch condition jump past increment
                const auto increment_begin_to_body_begin_jump_distance = translation_units.back().function->chunk.opcodes.size() - increment_begin_to_body_begin_jump_distance_offset - 2;
                if (increment_begin_to_body_begin_jump_distance > std::numeric_limits<std::uint16_t>::max()) {
                    throw Resumable_compiler_error{increment_expr_begin_token, "Too much code to jump over."};
                }
                translation_units.back().function->chunk.opcodes.at(increment_begin_to_body_begin_jump_distance_offset) = increment_begin_to_body_begin_jump_distance >> 8;
                translation_units.back().function->chunk.opcodes.at(increment_begin_to_body_begin_jump_distance_offset + 1) = increment_begin_to_body_begin_jump_distance & 0xff;
            }

            // Body
            const auto body_start_token = *token_iter;
            compile_statement();
            translation_units.back().emit(Opcode::loop, body_start_token);
            const auto body_end_to_increment_begin_jump_distance = translation_units.back().function->chunk.opcodes.size() - (increment_begin_offset ? *increment_begin_offset : condition_begin_offset) + 2;
            if (body_end_to_increment_begin_jump_distance > std::numeric_limits<std::uint16_t>::max()) {
                throw Resumable_compiler_error{body_start_token, "Loop body too large."};
            }
            translation_units.back().emit(body_end_to_increment_begin_jump_distance >> 8, body_end_to_increment_begin_jump_distance & 0xff, body_start_token);

            // Patch condition jump past body
            if (condition_false_jump_distance_offset) {
                const auto condition_false_jump_distance = translation_units.back().function->chunk.opcodes.size() - *condition_false_jump_distance_offset - 2;
                if (condition_false_jump_distance > std::numeric_limits<std::uint16_t>::max()) {
                    throw Resumable_compiler_error{body_start_token, "Too much code to jump over."};
                }
                translation_units.back().function->chunk.opcodes.at(*condition_false_jump_distance_offset) = condition_false_jump_distance >> 8;
                translation_units.back().function->chunk.opcodes.at(*condition_false_jump_distance_offset + 1) = condition_false_jump_distance & 0xff;

                // If condition is false, we'll land here, so pop condition expr result
                translation_units.back().emit(Opcode::pop, condition_expr_begin_token);
            }
        }

        void compile_if_statement() {
            // Condition
            consume(Token_type::left_paren, "Expected '(' after 'if'.");
            compile_expr_higher_precedence_than(Precedence_level::assignment);
            const auto right_paren_token = consume(Token_type::right_paren, "Expected ')' after condition.");
            translation_units.back().emit(Opcode::jump_if_false, right_paren_token);
            const auto condition_false_jump_distance_offset = translation_units.back().function->chunk.opcodes.size();
            translation_units.back().emit(0, 0, right_paren_token);

            // But if condition is true, pop expr result and fall through
            translation_units.back().emit(Opcode::pop, right_paren_token);
            const auto if_body_begin_begin_token = *token_iter;
            compile_statement();

            // After truthy body, jump over else body
            translation_units.back().emit(Opcode::jump, if_body_begin_begin_token);
            const auto if_body_end_to_else_end_jump_distance_offset = translation_units.back().function->chunk.opcodes.size();
            translation_units.back().emit(0, 0, if_body_begin_begin_token);

            // Patch condition jump to else
            const auto condition_false_jump_distance = translation_units.back().function->chunk.opcodes.size() - condition_false_jump_distance_offset - 2;
            if (condition_false_jump_distance > std::numeric_limits<std::uint16_t>::max()) {
                throw Resumable_compiler_error{if_body_begin_begin_token, "Too much code to jump over."};
            }
            translation_units.back().function->chunk.opcodes.at(condition_false_jump_distance_offset) = condition_false_jump_distance >> 8;
            translation_units.back().function->chunk.opcodes.at(condition_false_jump_distance_offset + 1) = condition_false_jump_distance & 0xff;

            // If condition is false, we'll land here, so pop condition expr result
            translation_units.back().emit(Opcode::pop, right_paren_token);
            const auto else_body_begin_token = *token_iter;
            if (consume_if(Token_type::else_)) {
                compile_statement();
            }

            // Patch jump over else body
            const auto if_body_end_to_else_end_jump_distance = translation_units.back().function->chunk.opcodes.size() - if_body_end_to_else_end_jump_distance_offset - 2;
            if (if_body_end_to_else_end_jump_distance > std::numeric_limits<std::uint16_t>::max()) {
                throw Resumable_compiler_error{else_body_begin_token, "Too much code to jump over."};
            }
            translation_units.back().function->chunk.opcodes.at(if_body_end_to_else_end_jump_distance_offset) = if_body_end_to_else_end_jump_distance >> 8;
            translation_units.back().function->chunk.opcodes.at(if_body_end_to_else_end_jump_distance_offset + 1) = if_body_end_to_else_end_jump_distance & 0xff;
        }

        void compile_return_statement(const Token& return_token) {
            if (translation_units.back().fn_type == Function_type::script) {
                throw Resumable_compiler_error{return_token, "Cannot return from top-level code."};
            }

            if (consume_if(Token_type::semicolon)) {
                if (translation_units.back().fn_type == Function_type::initializer) {
                    translation_units.back().emit(Opcode::get_local, translation_units.back().local_index("this"), *token_iter);
                } else {
                    translation_units.back().emit(Opcode::nil, *token_iter);
                }
                translation_units.back().emit(Opcode::return_, *token_iter);
            } else {
                if (translation_units.back().fn_type == Function_type::initializer) {
                    throw Resumable_compiler_error{return_token, "Cannot return a value from an initializer."};
                }

                compile_expr_higher_precedence_than(Precedence_level::assignment);
                consume(Token_type::semicolon, "Expected ';' after return value.");
                translation_units.back().emit(Opcode::return_, return_token);
            }
        }

        void compile_break_statement(const Token& break_token) {
            consume(Token_type::semicolon, "Expected ';' after break.");

            translation_units.back().emit(Opcode::jump, break_token);
            const auto to_end_jump_distance_offset = translation_units.back().function->chunk.opcodes.size();
            translation_units.back().emit(0, 0, break_token);

            tracked_loops.back().on_loop_emit_end.push_back([this, to_end_jump_distance_offset] () {
                // Patch jump to end
                const auto to_end_jump_distance = translation_units.back().function->chunk.opcodes.size() - to_end_jump_distance_offset - 2;
                translation_units.back().function->chunk.opcodes.at(to_end_jump_distance_offset) = to_end_jump_distance >> 8;
                translation_units.back().function->chunk.opcodes.at(to_end_jump_distance_offset + 1) = to_end_jump_distance & 0xff;
            });
        }

        void compile_continue_statement(const Token& continue_token) {
            consume(Token_type::semicolon, "Expected ';' after break.");

            translation_units.back().emit(Opcode::loop, continue_token);
            const auto to_begin_jump_distance = translation_units.back().function->chunk.opcodes.size() - tracked_loops.back().loop_begin_offset + 2;
            translation_units.back().emit(to_begin_jump_distance >> 8, to_begin_jump_distance & 0xff, continue_token);
        }

        void compile_while_statement() {
            const auto loop_begin_offset = translation_units.back().function->chunk.opcodes.size();

            tracked_loops.push_back({loop_begin_offset});
            const auto _ = gsl::finally([&] () {
                tracked_loops.pop_back();
            });

            // Condition
            consume(Token_type::left_paren, "Expected '(' after 'while'.");
            compile_expr_higher_precedence_than(Precedence_level::assignment);
            const auto right_paren_token = consume(Token_type::right_paren, "Expected ')' after condition.");
            translation_units.back().emit(Opcode::jump_if_false, right_paren_token);
            const auto to_end_jump_distance_offset = translation_units.back().function->chunk.opcodes.size();
            translation_units.back().emit(0, 0, right_paren_token);

            const auto stmt_begin_token = *token_iter;
            tracked_loops.back().on_loop_emit_end.push_back([this, to_end_jump_distance_offset, stmt_begin_token] () {
                // Patch jump to end
                const auto to_end_jump_distance = translation_units.back().function->chunk.opcodes.size() - to_end_jump_distance_offset - 2;
                if (to_end_jump_distance > std::numeric_limits<std::uint16_t>::max()) {
                    throw Resumable_compiler_error{stmt_begin_token, "Too much code to jump over."};
                }
                translation_units.back().function->chunk.opcodes.at(to_end_jump_distance_offset) = to_end_jump_distance >> 8;
                translation_units.back().function->chunk.opcodes.at(to_end_jump_distance_offset + 1) = to_end_jump_distance & 0xff;
            });

            // But if condition is true, pop expr result and fall through
            translation_units.back().emit(Opcode::pop, right_paren_token);
            compile_statement();
            translation_units.back().emit(Opcode::loop, stmt_begin_token);
            const auto to_begin_jump_distance = translation_units.back().function->chunk.opcodes.size() - loop_begin_offset + 2;
            if (to_begin_jump_distance > std::numeric_limits<std::uint16_t>::max()) {
                throw Resumable_compiler_error{stmt_begin_token, "Loop body too large."};
            }
            translation_units.back().emit(to_begin_jump_distance >> 8, to_begin_jump_distance & 0xff, stmt_begin_token);

            // If condition is false, we'll land here, so pop condition expr result
            translation_units.back().emit(Opcode::pop, right_paren_token);

            // At end of loop bytecode; notify listeners
            for (const auto& fn : tracked_loops.back().on_loop_emit_end) {
                fn();
            }
        }

        void compile_expression_statement() {
            compile_expr_higher_precedence_than(Precedence_level::assignment);
            const auto token = consume(Token_type::semicolon, "Expected ';' after expression.");
            translation_units.back().emit(Opcode::pop, token);
        }

        struct Precedence_level_fns {
            using Parsing_fn = void (Compiler::*)(const Token&, bool precedence_allows_assignment);

            Parsing_fn compile_prefix_expr_fn;
            Parsing_fn compile_infix_expr_fn;
            Precedence_level infix_expr_precedence;
        };

        // Pratt parser operator precedence table
        // Order is important; it must match the order in the Token_type enum
        std::array<Precedence_level_fns, 40> precedence_levels {{
            /* Token_type::left_paren    */ { &Compiler::compile_grouping, &Compiler::compile_call,   Precedence_level::call },
            /* Token_type::right_paren   */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::left_brace    */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::right_brace   */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::comma         */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::dot           */ { nullptr,                     &Compiler::compile_dot,    Precedence_level::call },
            /* Token_type::minus         */ { &Compiler::compile_unary,    &Compiler::compile_binary, Precedence_level::term },
            /* Token_type::plus          */ { nullptr,                     &Compiler::compile_binary, Precedence_level::term },
            /* Token_type::semicolon     */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::slash         */ { nullptr,                     &Compiler::compile_binary, Precedence_level::factor },
            /* Token_type::star          */ { nullptr,                     &Compiler::compile_binary, Precedence_level::factor },
            /* Token_type::bang          */ { &Compiler::compile_unary,    nullptr,                   Precedence_level::none },
            /* Token_type::bang_equal    */ { nullptr,                     &Compiler::compile_binary, Precedence_level::equality },
            /* Token_type::equal         */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::equal_equal   */ { nullptr,                     &Compiler::compile_binary, Precedence_level::equality },
            /* Token_type::greater       */ { nullptr,                     &Compiler::compile_binary, Precedence_level::comparison },
            /* Token_type::greater_equal */ { nullptr,                     &Compiler::compile_binary, Precedence_level::comparison },
            /* Token_type::less          */ { nullptr,                     &Compiler::compile_binary, Precedence_level::comparison },
            /* Token_type::less_equal    */ { nullptr,                     &Compiler::compile_binary, Precedence_level::comparison },
            /* Token_type::identifier    */ { &Compiler::compile_variable, nullptr,                   Precedence_level::none },
            /* Token_type::string        */ { &Compiler::compile_string,   nullptr,                   Precedence_level::none },
            /* Token_type::number        */ { &Compiler::compile_number,   nullptr,                   Precedence_level::none },
            /* Token_type::and_          */ { nullptr,                     &Compiler::compile_and,    Precedence_level::and_ },
            /* Token_type::class_        */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::else_         */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::false_        */ { &Compiler::compile_literal,  nullptr,                   Precedence_level::none },
            /* Token_type::for_          */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::fun_          */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::if_           */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::nil           */ { &Compiler::compile_literal,  nullptr,                   Precedence_level::none },
            /* Token_type::or_           */ { nullptr,                     &Compiler::compile_or,     Precedence_level::or_ },
            /* Token_type::print         */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::return_       */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::super_        */ { &Compiler::compile_super,    nullptr,                   Precedence_level::none },
            /* Token_type::this_         */ { &Compiler::compile_this,     nullptr,                   Precedence_level::none },
            /* Token_type::true_         */ { &Compiler::compile_literal,  nullptr,                   Precedence_level::none },
            /* Token_type::var_          */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::while_        */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::error         */ { nullptr,                     nullptr,                   Precedence_level::none },
            /* Token_type::eof           */ { nullptr,                     nullptr,                   Precedence_level::none },
        }};

        void compile_expr_higher_precedence_than(Precedence_level precedence) {
            const auto precedence_allows_assignment = precedence <= Precedence_level::assignment;

            const auto expr_begin_token = *token_iter++;
            const auto compile_prefix_expr_fn = precedence_levels.at(static_cast<int>(expr_begin_token.type)).compile_prefix_expr_fn;
            if (!compile_prefix_expr_fn) {
                throw Resumable_compiler_error{expr_begin_token, "Expected expression."};
            }
            (this->*compile_prefix_expr_fn)(expr_begin_token, precedence_allows_assignment);

            while (precedence <= precedence_levels.at(static_cast<int>(token_iter->type)).infix_expr_precedence) {
                const auto infix_expr_token = *token_iter++;
                const auto compile_infix_expr_fn = precedence_levels.at(static_cast<int>(infix_expr_token.type)).compile_infix_expr_fn;
                assert(compile_infix_expr_fn != nullptr);
                (this->*compile_infix_expr_fn)(infix_expr_token, precedence_allows_assignment);
            }

            // If we consumed the infix expression and there's still an equal sign,
            // then the infix expression wasn't assignable
            if (precedence_allows_assignment && token_iter->type == Token_type::equal) {
                throw Resumable_compiler_error{*token_iter, "Invalid assignment target."};
            }
        }

        void compile_literal(const Token& literal_token, bool /*precedence_allows_assignment*/) {
            switch (literal_token.type) {
                default:
                    throw "Unreachable";

                case Token_type::true_:
                    translation_units.back().emit(Opcode::true_, literal_token);
                    break;

                case Token_type::false_:
                    translation_units.back().emit(Opcode::false_, literal_token);
                    break;

                case Token_type::nil:
                    translation_units.back().emit(Opcode::nil, literal_token);
                    break;
            }
        }

        // NOTE: "compile this" must be read in a Schwarzenegger accent
        void compile_this(const Token& this_token, bool /*precedence_allows_assignment*/) {
            if (tracked_classes.empty()) {
                throw Resumable_compiler_error{this_token, "Cannot use 'this' outside of a class."};
            }
            compile_variable(this_token, false);
        }

        void compile_super(const Token& super_token, bool /*precedence_allows_assignment*/) {
            if (tracked_classes.empty()) {
                throw Resumable_compiler_error{super_token, "Cannot use 'super' outside of a class."};
            }
            if (!tracked_classes.back().has_superclass) {
                throw Resumable_compiler_error{super_token, "Cannot use 'super' in a class with no superclass."};
            }

            consume(Token_type::dot, "Expected '.' after 'super'.");
            const auto property_name_token = consume(Token_type::identifier, "Expected superclass method name.");
            translation_units.back().function->chunk.constants.push_back({interned_strings.get(property_name_token.lexeme)});
            const auto property_name_constant_index = translation_units.back().function->chunk.constants.size() - 1;

            compile_variable({Token_type::this_, "this"}, false);
            if (consume_if(Token_type::left_paren)) {
                const auto [arg_count, right_paren_token] = compile_argument_list();
                compile_variable({Token_type::super_, "super"}, false);
                translation_units.back().emit(Opcode::super_invoke, property_name_constant_index, property_name_token);
                translation_units.back().emit(arg_count, right_paren_token);
            } else {
                compile_variable({Token_type::super_, "super"}, false);
                translation_units.back().emit(Opcode::get_super, property_name_constant_index, property_name_token);
            }
        }

        std::tuple<std::uint8_t, Token> compile_argument_list() {
            auto arg_count = 0;

            if (token_iter->type != Token_type::right_paren) {
                do {
                    ++arg_count;
                    compile_expr_higher_precedence_than(Precedence_level::assignment);
                } while (consume_if(Token_type::comma));
            }
            if (arg_count > 8) {
                throw Resumable_compiler_error{*token_iter, "Cannot have more than 8 arguments."};
            }
            const auto right_paren_token = consume(Token_type::right_paren, "Expected ')' after arguments.");

            return {gsl::narrow<std::uint8_t>(arg_count), right_paren_token};
        }

        void compile_or(const Token& or_token, bool /*precedence_allows_assignment*/) {
            // Short circuit behavior means we need to jump to the RHS only if the LHS is false
            translation_units.back().emit(Opcode::jump_if_false, or_token);
            const auto to_rhs_jump_distance_offset = translation_units.back().function->chunk.opcodes.size();
            translation_units.back().emit(0, 0, or_token);

            // If the LHS was true, then we need to jump past the RHS
            translation_units.back().emit(Opcode::jump, or_token);
            const auto to_end_jump_distance_offset = translation_units.back().function->chunk.opcodes.size();
            translation_units.back().emit(0, 0, or_token);

            const auto to_rhs_jump_distance = translation_units.back().function->chunk.opcodes.size() - to_rhs_jump_distance_offset - 2;
            if (to_rhs_jump_distance > std::numeric_limits<std::uint16_t>::max()) {
                throw Resumable_compiler_error{or_token, "Too much code to jump over."};
            }
            translation_units.back().function->chunk.opcodes.at(to_rhs_jump_distance_offset) = to_rhs_jump_distance >> 8;
            translation_units.back().function->chunk.opcodes.at(to_rhs_jump_distance_offset + 1) = to_rhs_jump_distance & 0xff;

            // If the LHS was false, then the "or" expr value now depends solely on the RHS
            translation_units.back().emit(Opcode::pop, or_token);

            compile_expr_higher_precedence_than(Precedence_level::or_);
            const auto to_end_jump_distance = translation_units.back().function->chunk.opcodes.size() - to_end_jump_distance_offset - 2;
            if (to_end_jump_distance > std::numeric_limits<std::uint16_t>::max()) {
                throw Resumable_compiler_error{or_token, "Too much code to jump over."};
            }
            translation_units.back().function->chunk.opcodes.at(to_end_jump_distance_offset) = to_end_jump_distance >> 8;
            translation_units.back().function->chunk.opcodes.at(to_end_jump_distance_offset + 1) = to_end_jump_distance & 0xff;
        }

        void compile_and(const Token& and_token, bool /*precedence_allows_assignment*/) {
            // Short circuit behavior means we need to jump past the RHS if the LHS is false
            translation_units.back().emit(Opcode::jump_if_false, and_token);
            const auto jump_distance_offset = translation_units.back().function->chunk.opcodes.size();
            translation_units.back().emit(0, 0, and_token);

            // If the LHS was true, then the "and" expr value now depends solely on the RHS
            translation_units.back().emit(Opcode::pop, and_token);

            compile_expr_higher_precedence_than(Precedence_level::and_);

            const auto jump_distance = translation_units.back().function->chunk.opcodes.size() - jump_distance_offset - 2;
            if (jump_distance > std::numeric_limits<std::uint16_t>::max()) {
                throw Resumable_compiler_error{and_token, "Too much code to jump over."};
            }
            translation_units.back().function->chunk.opcodes.at(jump_distance_offset) = jump_distance >> 8;
            translation_units.back().function->chunk.opcodes.at(jump_distance_offset + 1) = jump_distance & 0xff;
        }

        void compile_number(const Token& number_token, bool /*precedence_allows_assignment*/) {
            const auto value = boost::lexical_cast<double>(gsl::to_string(number_token.lexeme));
            translation_units.back().function->chunk.constants.push_back({value});
            translation_units.back().emit(Opcode::constant, translation_units.back().function->chunk.constants.size() - 1, number_token);
        }

        void compile_string(const Token& string_token, bool /*precedence_allows_assignment*/) {
            translation_units.back().function->chunk.constants.push_back({interned_strings.get(
                gsl::cstring_span<>{&*(string_token.lexeme.begin() + 1), &*(string_token.lexeme.end() - 1)} // trim quotes
            )});
            translation_units.back().emit(Opcode::constant, translation_units.back().function->chunk.constants.size() - 1, string_token);
        }

        void compile_variable(const Token& variable_token, bool precedence_allows_assignment) {
            if (precedence_allows_assignment && consume_if(Token_type::equal)) {
                compile_expr_higher_precedence_than(Precedence_level::assignment);
                emit_setter(variable_token);
            } else {
                emit_getter(variable_token);
            }
        }

        void compile_binary(const Token& operator_token, bool /*precedence_allows_assignment*/) {
            // Right hand side
            const auto infix_expr_precedence = precedence_levels.at(static_cast<int>(operator_token.type)).infix_expr_precedence;
            const auto higher_than_infix_precedence = static_cast<Precedence_level>(static_cast<int>(infix_expr_precedence) + 1);
            compile_expr_higher_precedence_than(higher_than_infix_precedence);

            switch (operator_token.type) {
                default:
                    throw std::logic_error{"Unreachable"};

                case Token_type::bang_equal:
                    translation_units.back().emit(Opcode::equal, Opcode::not_, operator_token);
                    break;

                case Token_type::equal_equal:
                    translation_units.back().emit(Opcode::equal, operator_token);
                    break;

                case Token_type::greater:
                    translation_units.back().emit(Opcode::greater, operator_token);
                    break;

                case Token_type::greater_equal:
                    translation_units.back().emit(Opcode::less, Opcode::not_, operator_token);
                    break;

                case Token_type::less:
                    translation_units.back().emit(Opcode::less, operator_token);
                    break;

                case Token_type::less_equal:
                    translation_units.back().emit(Opcode::greater, Opcode::not_, operator_token);
                    break;

                case Token_type::plus:
                    translation_units.back().emit(Opcode::add, operator_token);
                    break;

                case Token_type::minus:
                    translation_units.back().emit(Opcode::subtract, operator_token);
                    break;

                case Token_type::star:
                    translation_units.back().emit(Opcode::multiply, operator_token);
                    break;

                case Token_type::slash:
                    translation_units.back().emit(Opcode::divide, operator_token);
                    break;
            }
        }

        void compile_unary(const Token& unary_token, bool /*precedence_allows_assignment*/) {
            compile_expr_higher_precedence_than(Precedence_level::unary);

            switch (unary_token.type) {
                default:
                    throw std::logic_error{"Unreachable"};

                case Token_type::bang:
                    translation_units.back().emit(Opcode::not_, unary_token);
                    break;

                case Token_type::minus:
                    translation_units.back().emit(Opcode::negate, unary_token);
                    break;
            }
        }

        void compile_dot(const Token& /*object_token*/, bool precedence_allows_assignment) {
            const auto property_name_token = consume(Token_type::identifier, "Expected property name after '.'.");
            translation_units.back().function->chunk.constants.push_back({interned_strings.get(property_name_token.lexeme)});
            const auto property_name_constant_index = translation_units.back().function->chunk.constants.size() - 1;

            if (precedence_allows_assignment && consume_if(Token_type::equal)) {
                compile_expr_higher_precedence_than(Precedence_level::assignment);
                translation_units.back().emit(Opcode::set_property, property_name_constant_index, property_name_token);
            } else if (consume_if(Token_type::left_paren)) {
                const auto [arg_count, right_paren_token] = compile_argument_list();
                translation_units.back().emit(Opcode::invoke, property_name_constant_index, property_name_token);
                translation_units.back().emit(arg_count, right_paren_token);
            } else {
                translation_units.back().emit(Opcode::get_property, property_name_constant_index, property_name_token);
            }
        }

        void compile_call(const Token& /*callee_token*/, bool /*precedence_allows_assignment*/) {
            const auto [arg_count, right_paren_token] = compile_argument_list();
            translation_units.back().emit(Opcode::call, arg_count, right_paren_token);
        }

        void compile_grouping(const Token& /*left_paren_token*/, bool /*precedence_allows_assignment*/) {
            compile_expr_higher_precedence_than(Precedence_level::assignment);
            consume(Token_type::right_paren, "Expected ')' after expression.");
        }
    };
}

namespace motts { namespace lox {
    GC_ptr<Closure> compile(GC_heap& gc_heap, Interned_strings& interned_strings, const gsl::cstring_span<>& source) {
        std::string compiler_errors;
        Compiler compiler {gc_heap, interned_strings, Token_iterator{source}, [&] (const std::exception& error) {
            compiler_errors += error.what();
            compiler_errors += '\n';
        }};

        while (compiler.token_iter->type != Token_type::eof) {
            compiler.compile_declaration();
        }
        compiler.translation_units.back().emit(Opcode::nil, Opcode::return_, *compiler.token_iter);

        if (!compiler_errors.empty()) {
            // Trim the extraneous training newline
            compiler_errors.erase(compiler_errors.end() - 1);

            throw std::runtime_error{compiler_errors};
        }

        return gc_heap.make(Closure{compiler.translation_units.front().function});
    }
}}
