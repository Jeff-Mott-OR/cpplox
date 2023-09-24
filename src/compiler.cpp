#include "compiler.hpp"

#include <cassert>
#include <sstream>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <gsl/gsl>

#include "object.hpp"
#include "scanner.hpp"

// Not exported (internal linkage).
namespace {
    // Allow the internal linkage section to access names.
    using namespace motts::lox;

    void ensure_token_is(const Token& token, const Token_type& expected) {
        if (token.type != expected) {
            std::ostringstream os;
            os << "[Line " << token.line << "] Error: Expected " << expected << " at \"" << token.lexeme << "\".";
            throw std::runtime_error{os.str()};
        }
    }

    // Functions are members of a struct to avoid lots of manual argument passing.
    struct Compiler {
        GC_heap& gc_heap;
        Token_iterator token_iter;
        unsigned int scope_depth {0};

        struct Function_chunk {
            Chunk chunk;

            struct Tracked_local {
                Token name;
                unsigned int depth {0};
                bool initialized {true};
                bool is_captured {false};
            };
            std::vector<Tracked_local> tracked_locals;
            std::vector<Chunk::Tracked_upvalue> tracked_upvalues;
        };
        std::vector<Function_chunk> function_chunks {{}};

        Compiler(GC_heap& gc_heap_arg, std::string_view source)
            : gc_heap {gc_heap_arg},
              token_iter {source}
        {}

        Chunk compile() {
            Token_iterator token_iter_end;
            while (token_iter != token_iter_end) {
                compile_statement();
            }

            return std::move(function_chunks.back().chunk);
        }

        bool advance_if_match(const Token_type& type) {
            if (token_iter->type == type) {
                ++token_iter;
                return true;
            }

            return false;
        }

        bool is_global_scope() const {
            return scope_depth == 0 && function_chunks.size() == 1;
        }

        void pop_top_scope_depth(const Token& source_map_token) {
            while (
                ! function_chunks.back().tracked_locals.empty() &&
                function_chunks.back().tracked_locals.back().depth == scope_depth
            ) {
                if (function_chunks.back().tracked_locals.back().is_captured) {
                    function_chunks.back().chunk.emit<Opcode::close_upvalue>(source_map_token);
                } else {
                    function_chunks.back().chunk.emit<Opcode::pop>(source_map_token);
                }
                function_chunks.back().tracked_locals.pop_back();
            }
            --scope_depth;
        }

        void track_local(const Token& identifier_token, bool initialized = true) {
            assert(! is_global_scope() && "We don't track locals in the global scope.");

            const auto maybe_redeclared_iter = std::find_if(
                function_chunks.back().tracked_locals.cbegin(), function_chunks.back().tracked_locals.cend(),
                [&] (const auto& tracked_local) {
                    return tracked_local.depth == scope_depth && tracked_local.name.lexeme == identifier_token.lexeme;
                }
            );
            if (maybe_redeclared_iter != function_chunks.back().tracked_locals.cend()) {
                std::ostringstream os;
                os << "[Line " << identifier_token.line << "] Error at \"" << identifier_token.lexeme
                    << "\": Identifier with this name already declared in this scope.";
                throw std::runtime_error{os.str()};
            }

            function_chunks.back().tracked_locals.push_back({identifier_token, scope_depth, initialized});
        }

        std::vector<Chunk::Tracked_upvalue>::const_iterator track_upvalue(const Token& identifier_token) {
            // Walk up nested functions looking for a local.
            for (auto enclosing_fn_iter = function_chunks.rbegin() + 1; enclosing_fn_iter != function_chunks.rend(); ++enclosing_fn_iter) {
                const auto maybe_enclosing_local_iter = std::find_if(
                    enclosing_fn_iter->tracked_locals.rbegin(), enclosing_fn_iter->tracked_locals.rend(),
                    [&] (const auto& tracked_local) {
                        return tracked_local.name.lexeme == identifier_token.lexeme;
                    }
                );

                if (maybe_enclosing_local_iter != enclosing_fn_iter->tracked_locals.crend()) {
                    maybe_enclosing_local_iter->is_captured = true;

                    // First the "direct" capture level that points to an enclosing stack local.
                    auto enclosing_upvalue_index = [&] {
                        const auto enclosing_local_index = maybe_enclosing_local_iter.base() - 1 - enclosing_fn_iter->tracked_locals.cbegin();
                        const Chunk::Tracked_upvalue new_tracked_upvalue {true, gsl::narrow<unsigned int>(enclosing_local_index)};

                        const auto directly_capturing_fn_iter = enclosing_fn_iter.base();
                        const auto maybe_existing_upvalue_iter = std::find(
                            directly_capturing_fn_iter->tracked_upvalues.cbegin(), directly_capturing_fn_iter->tracked_upvalues.cend(),
                            new_tracked_upvalue
                        );
                        if (maybe_existing_upvalue_iter != directly_capturing_fn_iter->tracked_upvalues.cend()) {
                            return gsl::narrow<unsigned int>(maybe_existing_upvalue_iter - directly_capturing_fn_iter->tracked_upvalues.cbegin());
                        } else {
                            directly_capturing_fn_iter->tracked_upvalues.push_back(new_tracked_upvalue);
                            return gsl::narrow<unsigned int>(directly_capturing_fn_iter->tracked_upvalues.size() - 1);
                        }
                    }();

                    // Then walk back down the nested functions of indirect capture levels that point to an enclosing upvalue.
                    for (
                        auto indirectly_capturing_fn_iter = enclosing_fn_iter.base() + 1;
                        indirectly_capturing_fn_iter != function_chunks.cend();
                        ++indirectly_capturing_fn_iter
                    ) {
                        const Chunk::Tracked_upvalue new_tracked_upvalue {false, gsl::narrow<unsigned int>(enclosing_upvalue_index)};
                        const auto maybe_existing_upvalue_iter = std::find(
                            indirectly_capturing_fn_iter->tracked_upvalues.cbegin(), indirectly_capturing_fn_iter->tracked_upvalues.cend(),
                            new_tracked_upvalue
                        );
                        if (maybe_existing_upvalue_iter != indirectly_capturing_fn_iter->tracked_upvalues.cend()) {
                            enclosing_upvalue_index = gsl::narrow<unsigned int>(maybe_existing_upvalue_iter - indirectly_capturing_fn_iter->tracked_upvalues.cbegin());
                        } else {
                            indirectly_capturing_fn_iter->tracked_upvalues.push_back(new_tracked_upvalue);
                            enclosing_upvalue_index = gsl::narrow<unsigned int>(indirectly_capturing_fn_iter->tracked_upvalues.size() - 1);
                        }
                    }

                    return function_chunks.back().tracked_upvalues.cbegin() + enclosing_upvalue_index;
                }
            }

            return function_chunks.back().tracked_upvalues.cend();
        }

        void compile_primary_expression() {
            switch (token_iter->type) {
                default: {
                    std::ostringstream os;
                    os << "[Line " << token_iter->line << "] Error: Unexpected token \"" << token_iter->lexeme << "\".";
                    throw std::runtime_error{os.str()};
                }

                case Token_type::false_: {
                    function_chunks.back().chunk.emit<Opcode::false_>(*token_iter++);
                    break;
                }

                case Token_type::identifier:
                case Token_type::this_: {
                    const auto identifier_token = *token_iter++;
                    auto call_source_map_token = identifier_token;

                    const auto maybe_local_iter = std::find_if(
                        function_chunks.back().tracked_locals.crbegin(), function_chunks.back().tracked_locals.crend(),
                        [&] (const auto& tracked_local) {
                            return tracked_local.name.lexeme == identifier_token.lexeme;
                        }
                    );
                    if (maybe_local_iter != function_chunks.back().tracked_locals.crend()) {
                        const auto local_iter = maybe_local_iter.base() - 1;

                        if (! local_iter->initialized) {
                            std::ostringstream os;
                            os << "[Line " << identifier_token.line << "] Error at \"" << identifier_token.lexeme
                                << "\": Cannot read local variable in its own initializer.";
                            throw std::runtime_error{os.str()};
                        }

                        const auto tracked_local_stack_index = local_iter - function_chunks.back().tracked_locals.cbegin();
                        function_chunks.back().chunk.emit<Opcode::get_local>(tracked_local_stack_index, identifier_token);
                    } else {
                        const auto maybe_upvalue_iter = track_upvalue(identifier_token);
                        if (maybe_upvalue_iter != function_chunks.back().tracked_upvalues.cend()) {
                            const auto tracked_upvalue_index = maybe_upvalue_iter - function_chunks.back().tracked_upvalues.cbegin();
                            function_chunks.back().chunk.emit<Opcode::get_upvalue>(tracked_upvalue_index, identifier_token);
                        } else {
                            function_chunks.back().chunk.emit<Opcode::get_global>(identifier_token, identifier_token);
                        }
                    }

                    while (token_iter->type == Token_type::dot || token_iter->type == Token_type::left_paren) {
                        // Check for property access following identifier.
                        if (token_iter->type == Token_type::dot) {
                            ++token_iter;
                            ensure_token_is(*token_iter, Token_type::identifier);
                            const auto property_name_token = *token_iter++;
                            call_source_map_token = property_name_token;
                            function_chunks.back().chunk.emit<Opcode::get_property>(property_name_token, property_name_token);
                        }

                        // Check for function call following identifier.
                        if (token_iter->type == Token_type::left_paren) {
                            ++token_iter;

                            auto arg_count = 0;
                            if (token_iter->type == Token_type::right_paren) {
                                ++token_iter;
                            } else {
                                do {
                                    compile_assignment_precedence_expression();
                                    ++arg_count;
                                } while (advance_if_match(Token_type::comma));
                                ensure_token_is(*token_iter++, Token_type::right_paren);
                            }

                            function_chunks.back().chunk.emit_call(arg_count, call_source_map_token);
                        }
                    }

                    break;
                }

                case Token_type::left_paren: {
                    ++token_iter;
                    compile_assignment_precedence_expression();
                    ensure_token_is(*token_iter++, Token_type::right_paren);
                    break;
                }

                case Token_type::nil: {
                    function_chunks.back().chunk.emit<Opcode::nil>(*token_iter++);
                    break;
                }

                case Token_type::number: {
                    const auto number_value = boost::lexical_cast<double>(token_iter->lexeme);
                    function_chunks.back().chunk.emit_constant(Dynamic_type_value{number_value}, *token_iter++);
                    break;
                }

                case Token_type::string: {
                    auto string_value = std::string{token_iter->lexeme.cbegin() + 1, token_iter->lexeme.cend() - 1};
                    function_chunks.back().chunk.emit_constant(Dynamic_type_value{std::move(string_value)}, *token_iter++);
                    break;
                }

                case Token_type::true_: {
                    function_chunks.back().chunk.emit<Opcode::true_>(*token_iter++);
                    break;
                }
            }
        }

        void compile_unary_precedence_expression() {
            if (token_iter->type == Token_type::minus || token_iter->type == Token_type::bang) {
                const auto unary_op_token = *token_iter++;

                // Right expression.
                compile_unary_precedence_expression();

                switch (unary_op_token.type) {
                    default:
                        throw std::logic_error{"Unreachable."};

                    case Token_type::minus:
                        function_chunks.back().chunk.emit<Opcode::negate>(unary_op_token);
                        break;

                    case Token_type::bang:
                        function_chunks.back().chunk.emit<Opcode::not_>(unary_op_token);
                        break;
                }

                return;
            }

            compile_primary_expression();
        }

        void compile_multiplication_precedence_expression() {
            // Left expression.
            compile_unary_precedence_expression();

            while (token_iter->type == Token_type::star || token_iter->type == Token_type::slash) {
                const auto binary_op_token = *token_iter++;

                // Right expression.
                compile_unary_precedence_expression();

                switch (binary_op_token.type) {
                    default:
                        throw std::logic_error{"Unreachable."};

                    case Token_type::star:
                        function_chunks.back().chunk.emit<Opcode::multiply>(binary_op_token);
                        break;

                    case Token_type::slash:
                        function_chunks.back().chunk.emit<Opcode::divide>(binary_op_token);
                        break;
                }
            }
        }

        void compile_addition_precedence_expression() {
            // Left expression.
            compile_multiplication_precedence_expression();

            while (token_iter->type == Token_type::plus || token_iter->type == Token_type::minus) {
                const auto binary_op_token = *token_iter++;

                // Right expression.
                compile_multiplication_precedence_expression();

                switch (binary_op_token.type) {
                    default:
                        throw std::logic_error{"Unreachable."};

                    case Token_type::plus:
                        function_chunks.back().chunk.emit<Opcode::add>(binary_op_token);
                        break;

                    case Token_type::minus:
                        function_chunks.back().chunk.emit<Opcode::subtract>(binary_op_token);
                        break;
                }
            }
        }

        void compile_comparison_precedence_expression() {
            // Left expression.
            compile_addition_precedence_expression();

            while (
                token_iter->type == Token_type::less || token_iter->type == Token_type::less_equal ||
                token_iter->type == Token_type::greater || token_iter->type == Token_type::greater_equal
            ) {
                const auto comparison_token = *token_iter++;

                // Right expression.
                compile_addition_precedence_expression();

                switch (comparison_token.type) {
                    default:
                        throw std::logic_error{"Unreachable."};

                    case Token_type::less:
                        function_chunks.back().chunk.emit<Opcode::less>(comparison_token);
                        break;

                    case Token_type::less_equal:
                        function_chunks.back().chunk.emit<Opcode::greater>(comparison_token);
                        function_chunks.back().chunk.emit<Opcode::not_>(comparison_token);
                        break;

                    case Token_type::greater:
                        function_chunks.back().chunk.emit<Opcode::greater>(comparison_token);
                        break;

                    case Token_type::greater_equal:
                        function_chunks.back().chunk.emit<Opcode::less>(comparison_token);
                        function_chunks.back().chunk.emit<Opcode::not_>(comparison_token);
                        break;
                }
            }
        }

        void compile_equality_precedence_expression() {
            // Left expression.
            compile_comparison_precedence_expression();

            while (token_iter->type == Token_type::equal_equal || token_iter->type == Token_type::bang_equal) {
                const auto equality_token = *token_iter++;

                // Right expression.
                compile_comparison_precedence_expression();

                switch (equality_token.type) {
                    default:
                        throw std::logic_error{"Unreachable."};

                    case Token_type::equal_equal:
                        function_chunks.back().chunk.emit<Opcode::equal>(equality_token);
                        break;

                    case Token_type::bang_equal:
                        function_chunks.back().chunk.emit<Opcode::equal>(equality_token);
                        function_chunks.back().chunk.emit<Opcode::not_>(equality_token);
                        break;
                }
            }
        }

        void compile_and_precedence_expression() {
            // Left expression.
            compile_equality_precedence_expression();

            while (token_iter->type == Token_type::and_) {
                auto short_circuit_jump_backpatch = function_chunks.back().chunk.emit_jump_if_false(*token_iter);
                // If the LHS was true, then the expression now depends solely on the RHS, and we can discard the LHS.
                function_chunks.back().chunk.emit<Opcode::pop>(*token_iter++);

                // Right expression.
                compile_equality_precedence_expression();

                short_circuit_jump_backpatch.to_next_opcode();
            }
        }

        void compile_or_precedence_expression() {
            // Left expression.
            compile_and_precedence_expression();

            while (token_iter->type == Token_type::or_) {
                auto to_rhs_jump_backpatch = function_chunks.back().chunk.emit_jump_if_false(*token_iter);
                auto to_end_jump_backpatch = function_chunks.back().chunk.emit_jump(*token_iter);

                to_rhs_jump_backpatch.to_next_opcode();
                // If the LHS was false, then the expression now depends solely on the RHS, and we can discard the LHS.
                function_chunks.back().chunk.emit<Opcode::pop>(*token_iter++);

                // Right expression.
                compile_and_precedence_expression();

                to_end_jump_backpatch.to_next_opcode();
            }
        }

        void compile_assignment_precedence_expression() {
            if (token_iter->type == Token_type::identifier) {
                const auto variable_name_token = *token_iter;

                auto peek_ahead_iter = token_iter;
                ++peek_ahead_iter;

                // Check for property access following identifier.
                std::vector<Token> property_name_tokens;
                while (peek_ahead_iter->type == Token_type::dot) {
                    ++peek_ahead_iter;
                    ensure_token_is(*peek_ahead_iter, Token_type::identifier);
                    property_name_tokens.push_back(*peek_ahead_iter++);
                }

                if (peek_ahead_iter->type == Token_type::equal) {
                    token_iter = ++peek_ahead_iter;

                    // Right expression.
                    compile_assignment_precedence_expression();

                    const auto maybe_local_iter = std::find_if(
                        function_chunks.back().tracked_locals.crbegin(), function_chunks.back().tracked_locals.crend(),
                        [&] (const auto& tracked_local) {
                            return tracked_local.name.lexeme == variable_name_token.lexeme;
                        }
                    );

                    if (maybe_local_iter != function_chunks.back().tracked_locals.crend())
                    {
                        const auto local_iter = maybe_local_iter.base() - 1;
                        const auto tracked_local_stack_index = local_iter - function_chunks.back().tracked_locals.cbegin();
                        if (property_name_tokens.empty()) {
                            function_chunks.back().chunk.emit<Opcode::set_local>(tracked_local_stack_index, variable_name_token);
                        } else {
                            function_chunks.back().chunk.emit<Opcode::get_local>(tracked_local_stack_index, variable_name_token);
                        }
                    }
                    else
                    {
                        const auto maybe_upvalue_iter = track_upvalue(variable_name_token);
                        if (maybe_upvalue_iter != function_chunks.back().tracked_upvalues.cend())
                        {
                            const auto tracked_upvalue_index = maybe_upvalue_iter - function_chunks.back().tracked_upvalues.cbegin();
                            if (property_name_tokens.empty()) {
                                function_chunks.back().chunk.emit<Opcode::set_upvalue>(tracked_upvalue_index, variable_name_token);
                            } else {
                                function_chunks.back().chunk.emit<Opcode::get_upvalue>(tracked_upvalue_index, variable_name_token);
                            }
                        }
                        else
                        {
                            if (property_name_tokens.empty()) {
                                function_chunks.back().chunk.emit<Opcode::set_global>(variable_name_token, variable_name_token);
                            } else {
                                function_chunks.back().chunk.emit<Opcode::get_global>(variable_name_token, variable_name_token);
                            }
                        }
                    }

                    if (! property_name_tokens.empty()) {
                        for (
                            auto property_name_iter = property_name_tokens.cbegin();
                            property_name_iter != property_name_tokens.cend() - 1;
                            ++property_name_iter
                        ) {
                            function_chunks.back().chunk.emit<Opcode::get_property>(*property_name_iter, *property_name_iter);
                        }
                        function_chunks.back().chunk.emit<Opcode::set_property>(property_name_tokens.back(), property_name_tokens.back());
                    }

                    return;
                }
            }

            compile_or_precedence_expression();
        }

        void compile_expression_statement() {
            compile_assignment_precedence_expression();
            ensure_token_is(*token_iter, Token_type::semicolon);
            function_chunks.back().chunk.emit<Opcode::pop>(*token_iter++);
        }

        int compile_function_rest(const Token& source_map_token) {
            auto param_count = 0;

            ensure_token_is(*token_iter++, Token_type::left_paren);
            if (token_iter->type != Token_type::right_paren) {
                do {
                    ensure_token_is(*token_iter, Token_type::identifier);
                    track_local(*token_iter++);
                    ++param_count;
                } while (advance_if_match(Token_type::comma));
            }
            ensure_token_is(*token_iter++, Token_type::right_paren);

            ensure_token_is(*token_iter, Token_type::left_brace);
            compile_statement();

            // A default return value.
            function_chunks.back().chunk.emit<Opcode::nil>(source_map_token);
            function_chunks.back().chunk.emit<Opcode::return_>(source_map_token);

            return param_count;
        }

        void compile_statement() {
            switch (token_iter->type) {
                default: {
                    compile_expression_statement();
                    break;
                }

                case Token_type::class_: {
                    const auto class_token = *token_iter++;

                    ensure_token_is(*token_iter, Token_type::identifier);
                    const auto class_name_token = *token_iter++;
                    function_chunks.back().chunk.emit<Opcode::class_>(class_name_token, class_token);

                    if (! is_global_scope()) {
                        track_local(class_name_token);
                    }

                    ensure_token_is(*token_iter++, Token_type::left_brace);
                    while (token_iter->type == Token_type::identifier) {
                        const auto method_name_token = *token_iter++;

                        function_chunks.push_back({});
                        track_local(Token{Token_type::this_, "this", method_name_token.line});
                        const auto param_count = compile_function_rest(method_name_token);

                        (function_chunks.end() - 2)->chunk.emit_closure(
                            gc_heap.make<Function>({method_name_token.lexeme, param_count, std::move(function_chunks.back().chunk)}),
                            function_chunks.back().tracked_upvalues,
                            method_name_token
                        );
                        function_chunks.pop_back();

                        function_chunks.back().chunk.emit<Opcode::method>(method_name_token, method_name_token);
                    }
                    ensure_token_is(*token_iter++, Token_type::right_brace);

                    if (is_global_scope()) {
                        function_chunks.back().chunk.emit<Opcode::define_global>(class_name_token, class_token);
                    }

                    break;
                }

                case Token_type::if_: {
                    const auto if_token = *token_iter++;

                    ensure_token_is(*token_iter++, Token_type::left_paren);
                    compile_assignment_precedence_expression();
                    ensure_token_is(*token_iter++, Token_type::right_paren);

                    auto to_else_or_end_jump_backpatch = function_chunks.back().chunk.emit_jump_if_false(if_token);
                    function_chunks.back().chunk.emit<Opcode::pop>(if_token);
                    compile_statement();

                    if (token_iter->type == Token_type::else_) {
                        const auto else_token = *token_iter++;
                        auto to_end_jump_backpatch = function_chunks.back().chunk.emit_jump(else_token);

                        to_else_or_end_jump_backpatch.to_next_opcode();
                        function_chunks.back().chunk.emit<Opcode::pop>(if_token);
                        compile_statement();

                        to_end_jump_backpatch.to_next_opcode();
                    } else {
                        to_else_or_end_jump_backpatch.to_next_opcode();
                        function_chunks.back().chunk.emit<Opcode::pop>(if_token);
                    }

                    break;
                }

                case Token_type::for_: {
                    const auto for_token = *token_iter++;
                    ++scope_depth;

                    ensure_token_is(*token_iter++, Token_type::left_paren);
                    if (token_iter->type != Token_type::semicolon) {
                        if (token_iter->type == Token_type::var) {
                            compile_statement();
                        } else {
                            compile_expression_statement();
                        }
                    } else {
                        ++token_iter;
                    }

                    const auto condition_begin_bytecode_index = function_chunks.back().chunk.bytecode().size();
                    if (token_iter->type != Token_type::semicolon) {
                        compile_assignment_precedence_expression();
                        ensure_token_is(*token_iter++, Token_type::semicolon);
                    } else {
                        function_chunks.back().chunk.emit<Opcode::true_>(for_token);
                        ++token_iter;
                    }
                    auto to_end_jump_backpatch = function_chunks.back().chunk.emit_jump_if_false(for_token);
                    auto to_body_jump_backpatch = function_chunks.back().chunk.emit_jump(for_token);

                    const auto increment_begin_bytecode_index = function_chunks.back().chunk.bytecode().size();
                    if (token_iter->type != Token_type::right_paren) {
                        compile_assignment_precedence_expression();
                        function_chunks.back().chunk.emit<Opcode::pop>(for_token);
                    }
                    ensure_token_is(*token_iter++, Token_type::right_paren);
                    function_chunks.back().chunk.emit_loop(condition_begin_bytecode_index, for_token);

                    to_body_jump_backpatch.to_next_opcode();
                    function_chunks.back().chunk.emit<Opcode::pop>(for_token);
                    compile_statement();
                    function_chunks.back().chunk.emit_loop(increment_begin_bytecode_index, for_token);

                    to_end_jump_backpatch.to_next_opcode();
                    function_chunks.back().chunk.emit<Opcode::pop>(for_token);

                    pop_top_scope_depth(for_token);

                    break;
                }

                case Token_type::fun: {
                    const auto fun_token = *token_iter++;
                    ensure_token_is(*token_iter, Token_type::identifier);
                    const auto fun_name_token = *token_iter++;

                    function_chunks.push_back({});
                    track_local(fun_name_token);
                    const auto param_count = compile_function_rest(fun_token);

                    (function_chunks.end() - 2)->chunk.emit_closure(
                        gc_heap.make<Function>({fun_name_token.lexeme, param_count, std::move(function_chunks.back().chunk)}),
                        function_chunks.back().tracked_upvalues,
                        fun_token
                    );
                    function_chunks.pop_back();

                    if (is_global_scope()) {
                        function_chunks.back().chunk.emit<Opcode::define_global>(fun_name_token, fun_token);
                    } else {
                        track_local(fun_name_token);
                    }

                    break;
                }

                case Token_type::left_brace: {
                    ++token_iter;
                    ++scope_depth;

                    Token_iterator token_iter_end;
                    while (token_iter != token_iter_end && token_iter->type != Token_type::right_brace) {
                        compile_statement();
                    }
                    ensure_token_is(*token_iter, Token_type::right_brace);
                    pop_top_scope_depth(*token_iter++);

                    break;
                }

                case Token_type::print: {
                    const auto print_token = *token_iter++;

                    compile_assignment_precedence_expression();
                    ensure_token_is(*token_iter++, Token_type::semicolon);
                    function_chunks.back().chunk.emit<Opcode::print>(print_token);

                    break;
                }

                case Token_type::return_: {
                    const auto return_token = *token_iter++;

                    if (token_iter->type != Token_type::semicolon) {
                        compile_assignment_precedence_expression();
                    } else {
                        function_chunks.back().chunk.emit<Opcode::nil>(return_token);
                    }
                    ensure_token_is(*token_iter++, Token_type::semicolon);
                    function_chunks.back().chunk.emit<Opcode::return_>(return_token);

                    break;
                }

                case Token_type::var: {
                    const auto var_token = *token_iter++;

                    ensure_token_is(*token_iter, Token_type::identifier);
                    const auto variable_name_token = *token_iter++;
                    if (! is_global_scope()) {
                        track_local(variable_name_token, false);
                    }

                    if (token_iter->type == Token_type::equal) {
                        ++token_iter;
                        compile_assignment_precedence_expression();
                    } else {
                        function_chunks.back().chunk.emit<Opcode::nil>(var_token);
                    }
                    ensure_token_is(*token_iter++, Token_type::semicolon);

                    if (is_global_scope()) {
                        function_chunks.back().chunk.emit<Opcode::define_global>(variable_name_token, var_token);
                    } else {
                        function_chunks.back().tracked_locals.back().initialized = true;
                    }

                    break;
                }

                case Token_type::while_: {
                    const auto while_token = *token_iter++;
                    const auto loop_begin_bytecode_index = function_chunks.back().chunk.bytecode().size();

                    ensure_token_is(*token_iter++, Token_type::left_paren);
                    compile_assignment_precedence_expression();
                    ensure_token_is(*token_iter++, Token_type::right_paren);

                    auto to_end_jump_backpatch = function_chunks.back().chunk.emit_jump_if_false(while_token);
                    function_chunks.back().chunk.emit<Opcode::pop>(while_token);

                    compile_statement();

                    function_chunks.back().chunk.emit_loop(loop_begin_bytecode_index, while_token);
                    to_end_jump_backpatch.to_next_opcode();
                    function_chunks.back().chunk.emit<Opcode::pop>(while_token);

                    break;
                }
            }
        }
    };
}

namespace motts { namespace lox {
    Chunk compile(GC_heap& gc_heap, std::string_view source) {
        return Compiler{gc_heap, source}.compile();
    }
}}
