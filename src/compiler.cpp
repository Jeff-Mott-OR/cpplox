#include "compiler.hpp"

#include <cassert>
#include <sstream>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <gsl/gsl>

#include "object.hpp"
#include "scanner.hpp"

namespace motts::lox
{
    static void ensure_token_is(const Token& token, Token_type expected)
    {
        if (token.type != expected) {
            std::ostringstream os;
            os << "[Line " << token.line << "] Error at \"" << token.lexeme << "\": Expected " << expected << '.';
            throw std::runtime_error{os.str()};
        }
    }

    // Functions are members of a struct to avoid lots of manual argument passing.
    struct Compiler
    {
        GC_heap& gc_heap;
        Interned_strings& interned_strings;
        Token_iterator token_iter;
        unsigned int scope_depth{0};

        struct Tracked_local
        {
            std::string_view name;
            unsigned int depth{0};
            bool initialized{true};
            bool is_captured{false};
        };

        struct Function_chunk
        {
            Chunk chunk;
            std::vector<Tracked_local> tracked_locals;
            std::vector<Tracked_upvalue> tracked_upvalues;
            bool is_class_init_method{false};
        };

        // The function chunk objects will be local variables that live on the stack,
        // and this vector of pointers merely gives a convenient way to iterate through them.
        std::vector<Function_chunk*> function_chunks;

        Compiler(GC_heap& gc_heap_arg, Interned_strings& interned_strings_arg, std::string_view source)
            : gc_heap{gc_heap_arg},
              interned_strings{interned_strings_arg},
              token_iter{source}
        {
        }

        Chunk compile()
        {
            Function_chunk root_chunk;
            function_chunks.push_back(&root_chunk);
            const auto _ = gsl::finally([&] { function_chunks.pop_back(); });

            Token_iterator token_iter_end;
            while (token_iter != token_iter_end) {
                compile_declaration();
            }

            return std::move(root_chunk.chunk);
        }

        bool advance_if_match(Token_type type)
        {
            if (token_iter->type == type) {
                ++token_iter;
                return true;
            }

            return false;
        }

        // The work we do to get or set a variable is the same but for the opcodes we emit.
        // And so a getter can instantiate this template with the getter opcodes, and a setter with the setter opcodes.
        template<Opcode local_opcode, Opcode upvalue_opcode, Opcode global_opcode>
        void emit_getter_setter(const Source_map_token& identifier_token)
        {
            const auto maybe_local_iter = std::find_if(
                function_chunks.back()->tracked_locals.crbegin(),
                function_chunks.back()->tracked_locals.crend(),
                [&](const auto& tracked_local) { return tracked_local.name == *identifier_token.lexeme; }
            );
            if (maybe_local_iter != function_chunks.back()->tracked_locals.crend()) {
                const auto local_iter = maybe_local_iter.base() - 1;

                if (! local_iter->initialized) {
                    std::ostringstream os;
                    os << "[Line " << identifier_token.line << "] Error at \"" << *identifier_token.lexeme
                       << "\": Cannot read local variable in its own initializer.";
                    throw std::runtime_error{os.str()};
                }

                const auto tracked_local_stack_index = local_iter - function_chunks.back()->tracked_locals.cbegin();
                function_chunks.back()->chunk.emit<local_opcode>(tracked_local_stack_index, identifier_token);
            } else {
                const auto maybe_upvalue_iter = track_upvalue(*identifier_token.lexeme);
                if (maybe_upvalue_iter != function_chunks.back()->tracked_upvalues.cend()) {
                    const auto tracked_upvalue_index = maybe_upvalue_iter - function_chunks.back()->tracked_upvalues.cbegin();
                    function_chunks.back()->chunk.emit<upvalue_opcode>(tracked_upvalue_index, identifier_token);
                } else {
                    function_chunks.back()->chunk.emit<global_opcode>(identifier_token.lexeme, identifier_token);
                }
            }
        }

        // Emit one of get_local, get_upvalue, or get_global, depending on where identifier was declared.
        void emit_getter(const Source_map_token& identifier_token)
        {
            emit_getter_setter<Opcode::get_local, Opcode::get_upvalue, Opcode::get_global>(identifier_token);
        }

        // Emit one of set_local, set_upvalue, or set_global, depending on where identifier was declared.
        void emit_setter(const Source_map_token& identifier_token)
        {
            emit_getter_setter<Opcode::set_local, Opcode::set_upvalue, Opcode::set_global>(identifier_token);
        }

        void pop_top_scope_depth(const Source_map_token& token)
        {
            while (! function_chunks.back()->tracked_locals.empty() && function_chunks.back()->tracked_locals.back().depth == scope_depth) {
                if (function_chunks.back()->tracked_locals.back().is_captured) {
                    function_chunks.back()->chunk.emit<Opcode::close_upvalue>(token);
                } else {
                    function_chunks.back()->chunk.emit<Opcode::pop>(token);
                }
                function_chunks.back()->tracked_locals.pop_back();
            }
            --scope_depth;
        }

        Source_map_token source_map_token(const Token& scanner_token)
        {
            return Source_map_token{interned_strings.get(scanner_token.lexeme), scanner_token.line};
        }

        void track_local(const Source_map_token& identifier_token, bool initialized = true)
        {
            assert(scope_depth > 0 && "We don't track locals in the global scope.");

            const auto maybe_redeclared_iter = std::find_if(
                function_chunks.back()->tracked_locals.cbegin(),
                function_chunks.back()->tracked_locals.cend(),
                [&](const auto& tracked_local) {
                    return tracked_local.depth == scope_depth && tracked_local.name == *identifier_token.lexeme;
                }
            );
            if (maybe_redeclared_iter != function_chunks.back()->tracked_locals.cend()) {
                std::ostringstream os;
                os << "[Line " << identifier_token.line << "] Error at \"" << *identifier_token.lexeme
                   << "\": Identifier with this name already declared in this scope.";
                throw std::runtime_error{os.str()};
            }

            function_chunks.back()->tracked_locals.push_back({*identifier_token.lexeme, scope_depth, initialized});
        }

        std::vector<Tracked_upvalue>::const_iterator track_upvalue(std::string_view identifier_name)
        {
            for (auto enclosing_fn_iter = function_chunks.rbegin() + 1; enclosing_fn_iter != function_chunks.rend(); ++enclosing_fn_iter) {
                const auto maybe_enclosing_local_iter = std::find_if(
                    (*enclosing_fn_iter)->tracked_locals.begin(),
                    (*enclosing_fn_iter)->tracked_locals.end(),
                    [&](const auto& tracked_local) { return tracked_local.name == identifier_name; }
                );

                if (maybe_enclosing_local_iter != (*enclosing_fn_iter)->tracked_locals.cend()) {
                    maybe_enclosing_local_iter->is_captured = true;

                    // First the "direct" capture level that points to an enclosing stack local.
                    auto enclosing_upvalue_index = [&] {
                        const auto enclosing_local_index = maybe_enclosing_local_iter - (*enclosing_fn_iter)->tracked_locals.cbegin();
                        const Tracked_upvalue new_tracked_upvalue{Upvalue_index{gsl::narrow<unsigned int>(enclosing_local_index)}};

                        const auto directly_capturing_fn_iter = enclosing_fn_iter.base();
                        const auto maybe_existing_upvalue_iter = std::find(
                            (*directly_capturing_fn_iter)->tracked_upvalues.cbegin(),
                            (*directly_capturing_fn_iter)->tracked_upvalues.cend(),
                            new_tracked_upvalue
                        );
                        if (maybe_existing_upvalue_iter != (*directly_capturing_fn_iter)->tracked_upvalues.cend()) {
                            const auto upvalue_index = gsl::narrow<unsigned int>(
                                maybe_existing_upvalue_iter - (*directly_capturing_fn_iter)->tracked_upvalues.cbegin()
                            );
                            return upvalue_index;
                        } else {
                            const auto upvalue_index = gsl::narrow<unsigned int>((*directly_capturing_fn_iter)->tracked_upvalues.size());
                            (*directly_capturing_fn_iter)->tracked_upvalues.push_back(new_tracked_upvalue);
                            return upvalue_index;
                        }
                    }();

                    // Then walk back down the nested functions of indirect capture levels that point to an enclosing upvalue.
                    for (auto indirectly_capturing_fn_iter = enclosing_fn_iter.base() + 1;
                         indirectly_capturing_fn_iter != function_chunks.cend();
                         ++indirectly_capturing_fn_iter)
                    {
                        const Tracked_upvalue new_tracked_upvalue{UpUpvalue_index{gsl::narrow<unsigned int>(enclosing_upvalue_index)}};
                        const auto maybe_existing_upvalue_iter = std::find(
                            (*indirectly_capturing_fn_iter)->tracked_upvalues.cbegin(),
                            (*indirectly_capturing_fn_iter)->tracked_upvalues.cend(),
                            new_tracked_upvalue
                        );
                        if (maybe_existing_upvalue_iter != (*indirectly_capturing_fn_iter)->tracked_upvalues.cend()) {
                            enclosing_upvalue_index = gsl::narrow<unsigned int>(
                                maybe_existing_upvalue_iter - (*indirectly_capturing_fn_iter)->tracked_upvalues.cbegin()
                            );
                        } else {
                            enclosing_upvalue_index = gsl::narrow<unsigned int>((*indirectly_capturing_fn_iter)->tracked_upvalues.size());
                            (*indirectly_capturing_fn_iter)->tracked_upvalues.push_back(new_tracked_upvalue);
                        }
                    }

                    return function_chunks.back()->tracked_upvalues.cbegin() + enclosing_upvalue_index;
                }
            }

            return function_chunks.back()->tracked_upvalues.cend();
        }

        void compile_primary_expression()
        {
            switch (token_iter->type) {
                default: {
                    std::ostringstream os;
                    os << "[Line " << token_iter->line << "] Error: Unexpected token \"" << token_iter->lexeme << "\".";
                    throw std::runtime_error{os.str()};
                }

                case Token_type::false_: {
                    function_chunks.back()->chunk.emit<Opcode::false_>(source_map_token(*token_iter++));
                    break;
                }

                case Token_type::fun: {
                    const auto fun_token = source_map_token(*token_iter++);

                    Function_chunk function_chunk;
                    function_chunks.push_back(&function_chunk);
                    ++scope_depth;
                    const auto _ = gsl::finally([&] {
                        --scope_depth;
                        function_chunks.pop_back();
                    });

                    const auto param_count = compile_function_rest(fun_token);

                    (*(function_chunks.end() - 2))
                        ->chunk.emit_closure(
                            gc_heap.make<Function>({interned_strings.get(""), param_count, std::move(function_chunk.chunk)}),
                            function_chunk.tracked_upvalues,
                            fun_token
                        );

                    break;
                }

                case Token_type::identifier:
                case Token_type::this_: {
                    emit_getter(source_map_token(*token_iter++));
                    break;
                }

                case Token_type::left_paren: {
                    ++token_iter;
                    compile_assignment_precedence_expression();
                    ensure_token_is(*token_iter++, Token_type::right_paren);

                    break;
                }

                case Token_type::nil: {
                    function_chunks.back()->chunk.emit<Opcode::nil>(source_map_token(*token_iter++));
                    break;
                }

                case Token_type::number: {
                    const auto number_value = boost::lexical_cast<double>(token_iter->lexeme);
                    function_chunks.back()->chunk.emit_constant(number_value, source_map_token(*token_iter++));

                    break;
                }

                case Token_type::string: {
                    const std::string_view quote_marks_trimmed{token_iter->lexeme.cbegin() + 1, token_iter->lexeme.cend() - 1};
                    function_chunks.back()->chunk.emit_constant(interned_strings.get(quote_marks_trimmed), source_map_token(*token_iter++));

                    break;
                }

                case Token_type::super: {
                    const auto super_token = source_map_token(*token_iter++);

                    emit_getter(source_map_token(Token{Token_type::this_, "this", super_token.line}));
                    emit_getter(super_token);

                    ensure_token_is(*token_iter++, Token_type::dot);
                    ensure_token_is(*token_iter, Token_type::identifier);
                    const auto method_name_token = source_map_token(*token_iter++);
                    function_chunks.back()->chunk.emit<Opcode::get_super>(method_name_token.lexeme, method_name_token);

                    break;
                }

                case Token_type::true_: {
                    function_chunks.back()->chunk.emit<Opcode::true_>(source_map_token(*token_iter++));
                    break;
                }
            }
        }

        void compile_call_precedence_expression()
        {
            auto callee_token = source_map_token(*token_iter);

            compile_primary_expression();

            while (token_iter->type == Token_type::left_paren || token_iter->type == Token_type::dot) {
                if (advance_if_match(Token_type::left_paren)) {
                    auto arg_count = 0;
                    if (! advance_if_match(Token_type::right_paren)) {
                        do {
                            compile_assignment_precedence_expression();
                            ++arg_count;
                        } while (advance_if_match(Token_type::comma));
                        ensure_token_is(*token_iter++, Token_type::right_paren);
                    }
                    function_chunks.back()->chunk.emit_call(arg_count, callee_token);
                }

                if (advance_if_match(Token_type::dot)) {
                    ensure_token_is(*token_iter, Token_type::identifier);
                    const auto property_name_token = source_map_token(*token_iter++);
                    function_chunks.back()->chunk.emit<Opcode::get_property>(property_name_token.lexeme, property_name_token);
                    callee_token = property_name_token;
                }
            }
        }

        void compile_unary_precedence_expression()
        {
            if (token_iter->type == Token_type::minus || token_iter->type == Token_type::bang) {
                const auto unary_op_scanner_token = *token_iter++;
                const auto unary_op_source_map_token = source_map_token(unary_op_scanner_token);

                // Right expression.
                compile_unary_precedence_expression();

                switch (unary_op_scanner_token.type) {
                    default:
                        throw std::logic_error{"Unreachable, probably."};

                    case Token_type::minus:
                        function_chunks.back()->chunk.emit<Opcode::negate>(unary_op_source_map_token);
                        break;

                    case Token_type::bang:
                        function_chunks.back()->chunk.emit<Opcode::not_>(unary_op_source_map_token);
                        break;
                }

                return;
            }

            compile_call_precedence_expression();
        }

        void compile_multiplication_precedence_expression()
        {
            // Left expression.
            compile_unary_precedence_expression();

            while (token_iter->type == Token_type::star || token_iter->type == Token_type::slash) {
                const auto binary_op_scanner_token = *token_iter++;
                const auto binary_op_source_map_token = source_map_token(binary_op_scanner_token);

                // Right expression.
                compile_unary_precedence_expression();

                switch (binary_op_scanner_token.type) {
                    default:
                        throw std::logic_error{"Unreachable, probably."};

                    case Token_type::star:
                        function_chunks.back()->chunk.emit<Opcode::multiply>(binary_op_source_map_token);
                        break;

                    case Token_type::slash:
                        function_chunks.back()->chunk.emit<Opcode::divide>(binary_op_source_map_token);
                        break;
                }
            }
        }

        void compile_addition_precedence_expression()
        {
            // Left expression.
            compile_multiplication_precedence_expression();

            while (token_iter->type == Token_type::plus || token_iter->type == Token_type::minus) {
                const auto binary_op_scanner_token = *token_iter++;
                const auto binary_op_source_map_token = source_map_token(binary_op_scanner_token);

                // Right expression.
                compile_multiplication_precedence_expression();

                switch (binary_op_scanner_token.type) {
                    default:
                        throw std::logic_error{"Unreachable, probably."};

                    case Token_type::plus:
                        function_chunks.back()->chunk.emit<Opcode::add>(binary_op_source_map_token);
                        break;

                    case Token_type::minus:
                        function_chunks.back()->chunk.emit<Opcode::subtract>(binary_op_source_map_token);
                        break;
                }
            }
        }

        void compile_comparison_precedence_expression()
        {
            // Left expression.
            compile_addition_precedence_expression();

            while (token_iter->type == Token_type::less || token_iter->type == Token_type::less_equal
                   || token_iter->type == Token_type::greater || token_iter->type == Token_type::greater_equal)
            {
                const auto comparison_scanner_token = *token_iter++;
                const auto comparison_source_map_token = source_map_token(comparison_scanner_token);

                // Right expression.
                compile_addition_precedence_expression();

                switch (comparison_scanner_token.type) {
                    default:
                        throw std::logic_error{"Unreachable, probably."};

                    case Token_type::less:
                        function_chunks.back()->chunk.emit<Opcode::less>(comparison_source_map_token);
                        break;

                    case Token_type::less_equal:
                        function_chunks.back()->chunk.emit<Opcode::greater>(comparison_source_map_token);
                        function_chunks.back()->chunk.emit<Opcode::not_>(comparison_source_map_token);

                        break;

                    case Token_type::greater:
                        function_chunks.back()->chunk.emit<Opcode::greater>(comparison_source_map_token);
                        break;

                    case Token_type::greater_equal:
                        function_chunks.back()->chunk.emit<Opcode::less>(comparison_source_map_token);
                        function_chunks.back()->chunk.emit<Opcode::not_>(comparison_source_map_token);

                        break;
                }
            }
        }

        void compile_equality_precedence_expression()
        {
            // Left expression.
            compile_comparison_precedence_expression();

            while (token_iter->type == Token_type::equal_equal || token_iter->type == Token_type::bang_equal) {
                const auto equality_scanner_token = *token_iter++;
                const auto equality_source_map_token = source_map_token(equality_scanner_token);

                // Right expression.
                compile_comparison_precedence_expression();

                switch (equality_scanner_token.type) {
                    default:
                        throw std::logic_error{"Unreachable, probably."};

                    case Token_type::equal_equal:
                        function_chunks.back()->chunk.emit<Opcode::equal>(equality_source_map_token);
                        break;

                    case Token_type::bang_equal:
                        function_chunks.back()->chunk.emit<Opcode::equal>(equality_source_map_token);
                        function_chunks.back()->chunk.emit<Opcode::not_>(equality_source_map_token);

                        break;
                }
            }
        }

        void compile_and_precedence_expression()
        {
            // Left expression.
            compile_equality_precedence_expression();

            while (token_iter->type == Token_type::and_) {
                const auto and_token = source_map_token(*token_iter++);

                auto short_circuit_jump_backpatch = function_chunks.back()->chunk.emit_jump_if_false(and_token);
                // If the LHS was true, then the expression now depends solely on the RHS, and we can discard the LHS.
                function_chunks.back()->chunk.emit<Opcode::pop>(and_token);

                // Right expression.
                compile_equality_precedence_expression();

                short_circuit_jump_backpatch.to_next_opcode();
            }
        }

        void compile_or_precedence_expression()
        {
            // Left expression.
            compile_and_precedence_expression();

            while (token_iter->type == Token_type::or_) {
                const auto or_token = source_map_token(*token_iter++);

                auto to_rhs_jump_backpatch = function_chunks.back()->chunk.emit_jump_if_false(or_token);
                auto to_end_jump_backpatch = function_chunks.back()->chunk.emit_jump(or_token);

                to_rhs_jump_backpatch.to_next_opcode();
                // If the LHS was false, then the expression now depends solely on the RHS, and we can discard the LHS.
                function_chunks.back()->chunk.emit<Opcode::pop>(or_token);

                // Right expression.
                compile_and_precedence_expression();

                to_end_jump_backpatch.to_next_opcode();
            }
        }

        void compile_assignment_precedence_expression()
        {
            if (token_iter->type == Token_type::identifier || token_iter->type == Token_type::this_) {
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
                    token_iter = peek_ahead_iter;
                    const auto equal_token = *token_iter++;

                    // Right expression.
                    compile_assignment_precedence_expression();

                    if (property_name_tokens.empty()) {
                        if (variable_name_token.lexeme == "this") {
                            throw std::runtime_error{
                                "[Line " + std::to_string(equal_token.line) + "] Error at \"=\": Invalid assignment target."};
                        }

                        emit_setter(source_map_token(variable_name_token));
                    } else {
                        emit_getter(source_map_token(variable_name_token));

                        for (auto property_name_iter = property_name_tokens.cbegin(); property_name_iter != property_name_tokens.cend() - 1;
                             ++property_name_iter) {
                            const auto property_name_token = source_map_token(*property_name_iter);
                            function_chunks.back()->chunk.emit<Opcode::get_property>(property_name_token.lexeme, property_name_token);
                        }

                        const auto last_property_name_token = source_map_token(property_name_tokens.back());
                        function_chunks.back()->chunk.emit<Opcode::set_property>(last_property_name_token.lexeme, last_property_name_token);
                    }

                    return;
                }
            }

            compile_or_precedence_expression();
        }

        void compile_expression_statement()
        {
            compile_assignment_precedence_expression();

            // Improve the error message a user sees for an invalid assignment.
            if (token_iter->type == Token_type::equal) {
                throw std::runtime_error{"[Line " + std::to_string(token_iter->line) + "] Error at \"=\": Invalid assignment target."};
            }
            ensure_token_is(*token_iter, Token_type::semicolon);

            function_chunks.back()->chunk.emit<Opcode::pop>(source_map_token(*token_iter++));
        }

        unsigned int compile_function_rest(const Source_map_token& fun_source_map_token)
        {
            unsigned int param_count{0};

            ensure_token_is(*token_iter++, Token_type::left_paren);
            if (! advance_if_match(Token_type::right_paren)) {
                do {
                    ensure_token_is(*token_iter, Token_type::identifier);
                    track_local(source_map_token(*token_iter++));
                    ++param_count;
                } while (advance_if_match(Token_type::comma));
                ensure_token_is(*token_iter++, Token_type::right_paren);
            }

            ensure_token_is(*token_iter++, Token_type::left_brace);
            Token_iterator token_iter_end;
            while (token_iter != token_iter_end && token_iter->type != Token_type::right_brace) {
                compile_declaration();
            }
            ensure_token_is(*token_iter++, Token_type::right_brace);

            // A default return value.
            if (function_chunks.back()->is_class_init_method) {
                function_chunks.back()->chunk.emit<Opcode::get_local>(
                    0,
                    source_map_token(Token{Token_type::this_, "this", fun_source_map_token.line})
                );
            } else {
                function_chunks.back()->chunk.emit<Opcode::nil>(fun_source_map_token);
            }
            function_chunks.back()->chunk.emit<Opcode::return_>(fun_source_map_token);

            return param_count;
        }

        void compile_statement()
        {
            switch (token_iter->type) {
                default: {
                    compile_expression_statement();
                    break;
                }

                case Token_type::if_: {
                    const auto if_token = source_map_token(*token_iter++);

                    ensure_token_is(*token_iter++, Token_type::left_paren);
                    compile_assignment_precedence_expression();
                    ensure_token_is(*token_iter++, Token_type::right_paren);

                    auto to_else_jump_backpatch = function_chunks.back()->chunk.emit_jump_if_false(if_token);
                    function_chunks.back()->chunk.emit<Opcode::pop>(if_token);
                    compile_statement();
                    auto to_end_jump_backpatch = function_chunks.back()->chunk.emit_jump(if_token);

                    to_else_jump_backpatch.to_next_opcode();
                    function_chunks.back()->chunk.emit<Opcode::pop>(if_token);
                    if (advance_if_match(Token_type::else_)) {
                        compile_statement();
                    }

                    to_end_jump_backpatch.to_next_opcode();

                    break;
                }

                case Token_type::for_: {
                    const auto for_token = source_map_token(*token_iter++);

                    ++scope_depth;
                    ensure_token_is(*token_iter++, Token_type::left_paren);
                    if (! advance_if_match(Token_type::semicolon)) {
                        if (token_iter->type == Token_type::var) {
                            compile_declaration();
                        } else {
                            compile_expression_statement();
                        }
                    }

                    const auto condition_begin_bytecode_index = function_chunks.back()->chunk.bytecode().size();
                    if (advance_if_match(Token_type::semicolon)) {
                        function_chunks.back()->chunk.emit<Opcode::true_>(for_token);
                    } else {
                        compile_assignment_precedence_expression();
                        ensure_token_is(*token_iter++, Token_type::semicolon);
                    }
                    auto to_end_jump_backpatch = function_chunks.back()->chunk.emit_jump_if_false(for_token);
                    auto to_body_jump_backpatch = function_chunks.back()->chunk.emit_jump(for_token);

                    const auto increment_begin_bytecode_index = function_chunks.back()->chunk.bytecode().size();
                    if (token_iter->type != Token_type::right_paren) {
                        compile_assignment_precedence_expression();
                        function_chunks.back()->chunk.emit<Opcode::pop>(for_token);
                    }
                    ensure_token_is(*token_iter++, Token_type::right_paren);
                    function_chunks.back()->chunk.emit_loop(condition_begin_bytecode_index, for_token);

                    to_body_jump_backpatch.to_next_opcode();
                    function_chunks.back()->chunk.emit<Opcode::pop>(for_token);
                    compile_statement();
                    function_chunks.back()->chunk.emit_loop(increment_begin_bytecode_index, for_token);

                    to_end_jump_backpatch.to_next_opcode();
                    function_chunks.back()->chunk.emit<Opcode::pop>(for_token);

                    pop_top_scope_depth(for_token);

                    break;
                }

                case Token_type::left_brace: {
                    ++token_iter;
                    ++scope_depth;

                    Token_iterator token_iter_end;
                    while (token_iter != token_iter_end && token_iter->type != Token_type::right_brace) {
                        compile_declaration();
                    }
                    ensure_token_is(*token_iter, Token_type::right_brace);
                    pop_top_scope_depth(source_map_token(*token_iter++));

                    break;
                }

                case Token_type::print: {
                    const auto print_token = source_map_token(*token_iter++);

                    compile_assignment_precedence_expression();
                    ensure_token_is(*token_iter++, Token_type::semicolon);
                    function_chunks.back()->chunk.emit<Opcode::print>(print_token);

                    break;
                }

                case Token_type::return_: {
                    if (function_chunks.size() == 1) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(token_iter->line) + "] Error at \"return\": Can't return from top-level code."};
                    }

                    const auto return_token = source_map_token(*token_iter++);
                    if (advance_if_match(Token_type::semicolon)) {
                        if (function_chunks.back()->is_class_init_method) {
                            function_chunks.back()->chunk.emit<Opcode::get_local>(
                                0,
                                source_map_token(Token{Token_type::this_, "this", return_token.line})
                            );
                        } else {
                            function_chunks.back()->chunk.emit<Opcode::nil>(return_token);
                        }
                    } else {
                        if (function_chunks.back()->is_class_init_method) {
                            throw std::runtime_error{
                                "[Line " + std::to_string(token_iter->line)
                                + "] Error at \"return\": Can't return a value from an initializer."};
                        }

                        compile_assignment_precedence_expression();
                        ensure_token_is(*token_iter++, Token_type::semicolon);
                    }
                    function_chunks.back()->chunk.emit<Opcode::return_>(return_token);

                    break;
                }

                case Token_type::while_: {
                    const auto while_token = source_map_token(*token_iter++);
                    const auto loop_begin_bytecode_index = function_chunks.back()->chunk.bytecode().size();

                    ensure_token_is(*token_iter++, Token_type::left_paren);
                    compile_assignment_precedence_expression();
                    ensure_token_is(*token_iter++, Token_type::right_paren);
                    auto to_end_jump_backpatch = function_chunks.back()->chunk.emit_jump_if_false(while_token);

                    function_chunks.back()->chunk.emit<Opcode::pop>(while_token);
                    compile_statement();
                    function_chunks.back()->chunk.emit_loop(loop_begin_bytecode_index, while_token);

                    to_end_jump_backpatch.to_next_opcode();
                    function_chunks.back()->chunk.emit<Opcode::pop>(while_token);

                    break;
                }
            }
        }

        void compile_declaration()
        {
            switch (token_iter->type) {
                default: {
                    compile_statement();
                    break;
                }

                case Token_type::class_: {
                    const auto class_token = source_map_token(*token_iter++);

                    ensure_token_is(*token_iter, Token_type::identifier);
                    const auto class_name_token = source_map_token(*token_iter++);
                    function_chunks.back()->chunk.emit<Opcode::class_>(class_name_token.lexeme, class_token);

                    if (scope_depth > 0) {
                        track_local(class_name_token);
                    }

                    std::function<void()> maybe_pop_superclass_scope;
                    if (advance_if_match(Token_type::less)) {
                        ensure_token_is(*token_iter, Token_type::identifier);
                        const auto superclass_name_token = source_map_token(*token_iter++);

                        if (superclass_name_token.lexeme == class_name_token.lexeme) {
                            throw std::runtime_error{
                                "[Line " + std::to_string(superclass_name_token.line) + "] Error: A class can't inherit from itself."};
                        }

                        if (scope_depth == 0) {
                            // The new class object is on the stack, and it will be there while we setup inheritance and methods.
                            // But on the global scope branch, we don't want to track that stack slot like a local,
                            // but we do want to track later values on the stack as locals. To make those slots line up,
                            // we need a placeholder to account for the stack slot that the class takes.
                            function_chunks.back()->tracked_locals.push_back({});
                        }

                        ++scope_depth;
                        emit_getter(superclass_name_token);
                        track_local(source_map_token(Token{Token_type::super, "super", superclass_name_token.line}));
                        function_chunks.back()->chunk.emit<Opcode::inherit>(superclass_name_token);

                        maybe_pop_superclass_scope = [this, superclass_name_token] {
                            // The inherit opcode puts the new class back on top of the stack, so that the subsequent method opcodes will
                            // operate on the new class. Now that we're done setting up the class, we need to pop that new class off.
                            function_chunks.back()->chunk.emit<Opcode::pop>(superclass_name_token);

                            // Super will either pop or close.
                            pop_top_scope_depth(source_map_token(Token{Token_type::super, "super", superclass_name_token.line}));

                            // Remove the placeholder tracked local.
                            if (scope_depth == 0) {
                                function_chunks.back()->tracked_locals.pop_back();
                            }
                        };
                    }

                    ensure_token_is(*token_iter++, Token_type::left_brace);
                    while (token_iter->type == Token_type::identifier) {
                        const auto method_name_token = source_map_token(*token_iter++);

                        Function_chunk function_chunk;
                        function_chunks.push_back(&function_chunk);
                        ++scope_depth;
                        const auto _ = gsl::finally([&] {
                            --scope_depth;
                            function_chunks.pop_back();
                        });
                        if (*method_name_token.lexeme == "init") {
                            function_chunk.is_class_init_method = true;
                        }

                        track_local(source_map_token(Token{Token_type::this_, "this", method_name_token.line}));
                        const auto param_count = compile_function_rest(method_name_token);

                        auto& enclosing_chunk = (*(function_chunks.end() - 2))->chunk;
                        enclosing_chunk.emit_closure(
                            gc_heap.make<Function>({method_name_token.lexeme, param_count, std::move(function_chunk.chunk)}),
                            function_chunk.tracked_upvalues,
                            method_name_token
                        );
                        enclosing_chunk.emit<Opcode::method>(method_name_token.lexeme, method_name_token);
                    }
                    ensure_token_is(*token_iter++, Token_type::right_brace);

                    if (maybe_pop_superclass_scope) {
                        maybe_pop_superclass_scope();
                    }

                    if (scope_depth == 0) {
                        function_chunks.back()->chunk.emit<Opcode::define_global>(class_name_token.lexeme, class_token);
                    }

                    break;
                }

                case Token_type::fun: {
                    const auto fun_token = source_map_token(*token_iter++);
                    ensure_token_is(*token_iter, Token_type::identifier);
                    const auto fun_name_token = source_map_token(*token_iter++);

                    {
                        Function_chunk function_chunk;
                        function_chunks.push_back(&function_chunk);
                        ++scope_depth;
                        const auto _ = gsl::finally([&] {
                            --scope_depth;
                            function_chunks.pop_back();
                        });

                        track_local(fun_name_token);
                        const auto param_count = compile_function_rest(fun_token);

                        (*(function_chunks.end() - 2))
                            ->chunk.emit_closure(
                                gc_heap.make<Function>({fun_name_token.lexeme, param_count, std::move(function_chunk.chunk)}),
                                function_chunk.tracked_upvalues,
                                fun_token
                            );
                    }

                    if (scope_depth == 0) {
                        function_chunks.back()->chunk.emit<Opcode::define_global>(fun_name_token.lexeme, fun_token);
                    } else {
                        track_local(fun_name_token);
                    }

                    break;
                }

                case Token_type::var: {
                    const auto var_token = source_map_token(*token_iter++);

                    ensure_token_is(*token_iter, Token_type::identifier);
                    const auto variable_name_token = source_map_token(*token_iter++);
                    if (scope_depth > 0) {
                        track_local(variable_name_token, false);
                    }

                    if (advance_if_match(Token_type::equal)) {
                        compile_assignment_precedence_expression();
                    } else {
                        function_chunks.back()->chunk.emit<Opcode::nil>(var_token);
                    }
                    ensure_token_is(*token_iter++, Token_type::semicolon);

                    if (scope_depth == 0) {
                        function_chunks.back()->chunk.emit<Opcode::define_global>(variable_name_token.lexeme, var_token);
                    } else {
                        function_chunks.back()->tracked_locals.back().initialized = true;
                    }

                    break;
                }
            }
        }
    };

    GC_ptr<Function> compile(GC_heap& gc_heap, Interned_strings& interned_strings, std::string_view source)
    {
        const auto root_script_fn =
            gc_heap.make<Function>({interned_strings.get(""), 0, Compiler{gc_heap, interned_strings, source}.compile()});
        return root_script_fn;
    }
}
