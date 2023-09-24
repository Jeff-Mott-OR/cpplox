#include "compiler.hpp"

#include <cstddef>
#include <iomanip>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/lexical_cast.hpp>
#include <gsl/gsl>

#include "scanner.hpp"

// Not exported (internal linkage)
namespace {
    // Allow the internal linkage section to access names
    using namespace motts::lox;

    struct Dynamic_type_print_visitor {
        std::ostream& os;
        Dynamic_type_print_visitor(std::ostream& os_arg)
            : os {os_arg}
        {
        }

        auto operator()(std::nullptr_t) {
            os << "nil";
        }

        auto operator()(bool value) {
            os << std::boolalpha << value;
        }

        auto operator()(Function* fn) {
            os << "<fn " << fn->name << ">";
        }

        template<typename T>
            auto operator()(const T& value) {
                os << value;
            }
    };

    void ensure_token_is(const Token& token, const Token_type& expected) {
        if (token.type != expected) {
            std::ostringstream os;
            os << "[Line " << token.line << "] Error: Expected " << expected << " at \"" << token.lexeme << "\".";
            throw std::runtime_error{os.str()};
        }
    }

    struct Tracked_local {
        Token name;
        int depth {0};
        bool initialized {true};
    };

    // There's no invariant being maintained here.
    // This exists primarily to avoid lots of manual argument passing.
    class Compiler {
        std::vector<Chunk> function_chunks_ {{}};
        Token_iterator token_iter_;
        std::vector<Tracked_local> tracked_locals_;
        int scope_depth_ {0};

        public:
            Compiler(std::string_view source)
                : token_iter_ {source}
            {
            }

            Chunk compile() {
                Token_iterator token_iter_end;
                while (token_iter_ != token_iter_end) {
                    compile_statement();
                }

                return std::move(function_chunks_.back());
            }

        private:
            auto pop_local_scope(const Token& source_map_token) {
                while (! tracked_locals_.empty() && tracked_locals_.back().depth == scope_depth_) {
                    tracked_locals_.pop_back();
                    function_chunks_.back().emit<Opcode::pop>(source_map_token);
                }
                --scope_depth_;
            }

            void compile_primary_expression() {
                switch (token_iter_->type) {
                    default: {
                        std::ostringstream os;
                        os << "[Line " << token_iter_->line << "] Error: Unexpected token \"" << token_iter_->lexeme << "\".";
                        throw std::runtime_error{os.str()};
                    }

                    case Token_type::false_: {
                        function_chunks_.back().emit<Opcode::false_>(*token_iter_++);
                        break;
                    }

                    case Token_type::identifier: {
                        const auto identifier_token = *token_iter_++;

                        const auto maybe_local_iter = std::find_if(
                            tracked_locals_.crbegin(), tracked_locals_.crend(),
                            [&] (const auto& tracked_local) {
                                return tracked_local.name.lexeme == identifier_token.lexeme;
                            }
                        );

                        if (maybe_local_iter != tracked_locals_.crend()) {
                            const auto local_iter = maybe_local_iter.base() - 1;

                            if (! local_iter->initialized) {
                                std::ostringstream os;
                                os << "[Line " << identifier_token.line << "] Error at \"" << identifier_token.lexeme
                                    << "\": Cannot read local variable in its own initializer.";
                                throw std::runtime_error{os.str()};
                            }

                            const auto tracked_local_stack_index = local_iter - tracked_locals_.cbegin();
                            function_chunks_.back().emit<Opcode::get_local>(tracked_local_stack_index, identifier_token);
                        } else {
                            function_chunks_.back().emit<Opcode::get_global>(identifier_token, identifier_token);
                        }

                        if (token_iter_->type == Token_type::left_paren) {
                            ++token_iter_;

                            auto arg_count = 0;
                            if (token_iter_->type == Token_type::right_paren) {
                                ++token_iter_;
                            } else {
                                do {
                                    ++arg_count;
                                    compile_assignment_precedence_expression();
                                } while (
                                    [&] {
                                        const auto is_comma_token = token_iter_->type == Token_type::comma;
                                        if (is_comma_token) {
                                            ++token_iter_;
                                        }
                                        return is_comma_token;
                                    }()
                                );
                                ensure_token_is(*token_iter_++, Token_type::right_paren);
                            }

                            function_chunks_.back().emit_call(arg_count, identifier_token);
                        }

                        break;
                    }

                    case Token_type::left_paren: {
                        ++token_iter_;
                        compile_assignment_precedence_expression();
                        ensure_token_is(*token_iter_++, Token_type::right_paren);

                        break;
                    }

                    case Token_type::nil: {
                        function_chunks_.back().emit<Opcode::nil>(*token_iter_++);
                        break;
                    }

                    case Token_type::number: {
                        const auto number_value = boost::lexical_cast<double>(token_iter_->lexeme);
                        function_chunks_.back().emit_constant(Dynamic_type_value{number_value}, *token_iter_++);

                        break;
                    }

                    case Token_type::string: {
                        auto string_value = std::string{token_iter_->lexeme.cbegin() + 1, token_iter_->lexeme.cend() - 1};
                        function_chunks_.back().emit_constant(Dynamic_type_value{std::move(string_value)}, *token_iter_++);

                        break;
                    }

                    case Token_type::true_: {
                        function_chunks_.back().emit<Opcode::true_>(*token_iter_++);
                        break;
                    }
                }
            }

            void compile_unary_precedence_expression() {
                if (token_iter_->type == Token_type::minus || token_iter_->type == Token_type::bang) {
                    const auto unary_op_token = *token_iter_++;

                    // Right expression
                    compile_unary_precedence_expression();

                    switch (unary_op_token.type) {
                        default:
                            throw std::logic_error{"Unreachable"};

                        case Token_type::minus:
                            function_chunks_.back().emit<Opcode::negate>(unary_op_token);
                            break;

                        case Token_type::bang:
                            function_chunks_.back().emit<Opcode::not_>(unary_op_token);
                            break;
                    }

                    return;
                }

                compile_primary_expression();
            }

            void compile_multiplication_precedence_expression() {
                // Left expression
                compile_unary_precedence_expression();

                while (token_iter_->type == Token_type::star || token_iter_->type == Token_type::slash) {
                    const auto binary_op_token = *token_iter_++;

                    // Right expression
                    compile_unary_precedence_expression();

                    switch (binary_op_token.type) {
                        default:
                            throw std::logic_error{"Unreachable"};

                        case Token_type::star:
                            function_chunks_.back().emit<Opcode::multiply>(binary_op_token);
                            break;

                        case Token_type::slash:
                            function_chunks_.back().emit<Opcode::divide>(binary_op_token);
                            break;
                    }
                }
            }

            void compile_addition_precedence_expression() {
                // Left expression
                compile_multiplication_precedence_expression();

                while (token_iter_->type == Token_type::plus || token_iter_->type == Token_type::minus) {
                    const auto binary_op_token = *token_iter_++;

                    // Right expression
                    compile_multiplication_precedence_expression();

                    switch (binary_op_token.type) {
                        default:
                            throw std::logic_error{"Unreachable"};

                        case Token_type::plus:
                            function_chunks_.back().emit<Opcode::add>(binary_op_token);
                            break;

                        case Token_type::minus:
                            function_chunks_.back().emit<Opcode::subtract>(binary_op_token);
                            break;
                    }
                }
            }

            void compile_comparison_precedence_expression() {
                // Left expression
                compile_addition_precedence_expression();

                while (
                    token_iter_->type == Token_type::less || token_iter_->type == Token_type::less_equal ||
                    token_iter_->type == Token_type::greater || token_iter_->type == Token_type::greater_equal
                ) {
                    const auto comparison_token = *token_iter_++;

                    // Right expression
                    compile_addition_precedence_expression();

                    switch (comparison_token.type) {
                        default:
                            throw std::logic_error{"Unreachable"};

                        case Token_type::less:
                            function_chunks_.back().emit<Opcode::less>(comparison_token);
                            break;

                        case Token_type::less_equal:
                            function_chunks_.back().emit<Opcode::greater>(comparison_token);
                            function_chunks_.back().emit<Opcode::not_>(comparison_token);
                            break;

                        case Token_type::greater:
                            function_chunks_.back().emit<Opcode::greater>(comparison_token);
                            break;

                        case Token_type::greater_equal:
                            function_chunks_.back().emit<Opcode::less>(comparison_token);
                            function_chunks_.back().emit<Opcode::not_>(comparison_token);
                            break;
                    }
                }
            }

            void compile_equality_precedence_expression() {
                // Left expression
                compile_comparison_precedence_expression();

                while (token_iter_->type == Token_type::equal_equal || token_iter_->type == Token_type::bang_equal) {
                    const auto equality_token = *token_iter_++;

                    // Right expression
                    compile_comparison_precedence_expression();

                    switch (equality_token.type) {
                        default:
                            throw std::logic_error{"Unreachable"};

                        case Token_type::equal_equal:
                            function_chunks_.back().emit<Opcode::equal>(equality_token);
                            break;

                        case Token_type::bang_equal:
                            function_chunks_.back().emit<Opcode::equal>(equality_token);
                            function_chunks_.back().emit<Opcode::not_>(equality_token);
                            break;
                    }
                }
            }

            void compile_and_precedence_expression() {
                // Left expression
                compile_equality_precedence_expression();

                while (token_iter_->type == Token_type::and_) {
                    auto short_circuit_jump_backpatch = function_chunks_.back().emit_jump_if_false(*token_iter_);
                    // If the LHS was true, then the expression now depends solely on the RHS, and we can discard the LHS
                    function_chunks_.back().emit<Opcode::pop>(*token_iter_++);

                    // Right expression
                    compile_equality_precedence_expression();

                    short_circuit_jump_backpatch.to_next_opcode();
                }
            }

            void compile_or_precedence_expression() {
                // Left expression
                compile_and_precedence_expression();

                while (token_iter_->type == Token_type::or_) {
                    auto to_rhs_jump_backpatch = function_chunks_.back().emit_jump_if_false(*token_iter_);
                    auto to_end_jump_backpatch = function_chunks_.back().emit_jump(*token_iter_);

                    to_rhs_jump_backpatch.to_next_opcode();
                    // If the LHS was false, then the expression now depends solely on the RHS, and we can discard the LHS
                    function_chunks_.back().emit<Opcode::pop>(*token_iter_++);

                    // Right expression
                    compile_and_precedence_expression();

                    to_end_jump_backpatch.to_next_opcode();
                }
            }

            void compile_assignment_precedence_expression() {
                if (token_iter_->type == Token_type::identifier) {
                    const auto variable_name_token = *token_iter_;

                    auto peek_ahead_iter = token_iter_;
                    ++peek_ahead_iter;

                    if (peek_ahead_iter->type == Token_type::equal) {
                        token_iter_ = peek_ahead_iter;
                        const auto assign_token = *token_iter_++;

                        // Right expression
                        compile_assignment_precedence_expression();

                        const auto maybe_local_iter = std::find_if(
                            tracked_locals_.crbegin(), tracked_locals_.crend(),
                            [&] (const auto& tracked_local) {
                                return tracked_local.name.lexeme == variable_name_token.lexeme;
                            }
                        );
                        if (maybe_local_iter != tracked_locals_.crend()) {
                            const auto local_iter = maybe_local_iter.base() - 1;
                            const auto tracked_local_stack_index = local_iter - tracked_locals_.cbegin();
                            function_chunks_.back().emit<Opcode::set_local>(tracked_local_stack_index, variable_name_token);
                        } else {
                            function_chunks_.back().emit<Opcode::set_global>(variable_name_token, assign_token);
                        }

                        return;
                    }
                }

                // Else
                compile_or_precedence_expression();
            }

            void compile_expression_statement() {
                compile_assignment_precedence_expression();
                ensure_token_is(*token_iter_, Token_type::semicolon);
                function_chunks_.back().emit<Opcode::pop>(*token_iter_++);
            }

            void compile_statement() {
                switch (token_iter_->type) {
                    default: {
                        compile_expression_statement();
                        break;
                    }

                    case Token_type::if_: {
                        const auto if_token = *token_iter_++;

                        ensure_token_is(*token_iter_++, Token_type::left_paren);
                        compile_assignment_precedence_expression();
                        ensure_token_is(*token_iter_++, Token_type::right_paren);

                        auto to_else_or_end_jump_backpatch = function_chunks_.back().emit_jump_if_false(if_token);
                        function_chunks_.back().emit<Opcode::pop>(if_token);
                        compile_statement();

                        if (token_iter_->type == Token_type::else_) {
                            const auto else_token = *token_iter_++;
                            auto to_end_jump_backpatch = function_chunks_.back().emit_jump(else_token);

                            to_else_or_end_jump_backpatch.to_next_opcode();
                            function_chunks_.back().emit<Opcode::pop>(if_token);
                            compile_statement();

                            to_end_jump_backpatch.to_next_opcode();
                        } else {
                            to_else_or_end_jump_backpatch.to_next_opcode();
                            function_chunks_.back().emit<Opcode::pop>(if_token);
                        }

                        break;
                    }

                    case Token_type::for_: {
                        const auto for_token = *token_iter_++;
                        ++scope_depth_;

                        ensure_token_is(*token_iter_++, Token_type::left_paren);
                        if (token_iter_->type != Token_type::semicolon) {
                            if (token_iter_->type == Token_type::var) {
                                compile_statement();
                            } else {
                                compile_expression_statement();
                            }
                        } else {
                            ++token_iter_;
                        }

                        const auto condition_begin_bytecode_index = function_chunks_.back().bytecode().size();
                        if (token_iter_->type != Token_type::semicolon) {
                            compile_assignment_precedence_expression();
                            ensure_token_is(*token_iter_++, Token_type::semicolon);
                        } else {
                            function_chunks_.back().emit<Opcode::true_>(for_token);
                            ++token_iter_;
                        }
                        auto to_end_jump_backpatch = function_chunks_.back().emit_jump_if_false(for_token);
                        auto to_body_jump_backpatch = function_chunks_.back().emit_jump(for_token);

                        const auto increment_begin_bytecode_index = function_chunks_.back().bytecode().size();
                        if (token_iter_->type != Token_type::right_paren) {
                            compile_assignment_precedence_expression();
                            function_chunks_.back().emit<Opcode::pop>(for_token);
                        }
                        ensure_token_is(*token_iter_++, Token_type::right_paren);
                        function_chunks_.back().emit_loop(condition_begin_bytecode_index, for_token);

                        to_body_jump_backpatch.to_next_opcode();
                        function_chunks_.back().emit<Opcode::pop>(for_token);
                        compile_statement();
                        function_chunks_.back().emit_loop(increment_begin_bytecode_index, for_token);

                        to_end_jump_backpatch.to_next_opcode();
                        function_chunks_.back().emit<Opcode::pop>(for_token);

                        pop_local_scope(for_token);

                        break;
                    }

                    case Token_type::fun: {
                        const auto fun_token = *token_iter_++;
                        ensure_token_is(*token_iter_, Token_type::identifier);
                        const auto fun_name_token = *token_iter_++;

                        ++scope_depth_;
                        // The caller already put the function on the stack. Within the function's body,
                        // we track that stack position and access it through the function's name identifier.
                        tracked_locals_.push_back({fun_name_token, scope_depth_});

                        ensure_token_is(*token_iter_++, Token_type::left_paren);
                        if (token_iter_->type == Token_type::right_paren) {
                            ++token_iter_;
                        } else {
                            do {
                                ensure_token_is(*token_iter_, Token_type::identifier);
                                const auto param_token = *token_iter_++;
                                // The caller already put the argument on the stack. Within the function's body,
                                // we track that stack position and access it through the parameter's name.
                                tracked_locals_.push_back({param_token, scope_depth_});
                            } while (
                                [&] {
                                    const auto is_comma_token = token_iter_->type == Token_type::comma;
                                    if (is_comma_token) {
                                        ++token_iter_;
                                    }
                                    return is_comma_token;
                                }()
                            );
                            ensure_token_is(*token_iter_++, Token_type::right_paren);
                        }

                        function_chunks_.push_back({});
                        ensure_token_is(*token_iter_, Token_type::left_brace);
                        compile_statement();
                        function_chunks_.back().emit<Opcode::nil>(fun_token);
                        function_chunks_.back().emit<Opcode::return_>(fun_token);

                        // Pop local scope but don't emit pop opcodes,
                        // because the return instruction will pop the whole frame.
                        while (! tracked_locals_.empty() && tracked_locals_.back().depth == scope_depth_) {
                            tracked_locals_.pop_back();
                        }
                        --scope_depth_;

                        auto fn = new Function{fun_name_token.lexeme, std::move(function_chunks_.back())};
                        function_chunks_.pop_back();
                        function_chunks_.back().emit_constant(Dynamic_type_value{fn}, fun_token);

                        if (scope_depth_ == 0) {
                            function_chunks_.back().emit<Opcode::define_global>(fun_name_token, fun_token);
                        } else {
                            tracked_locals_.push_back({fun_name_token, scope_depth_});
                        }

                        break;
                    }

                    case Token_type::left_brace: {
                        ++token_iter_;
                        ++scope_depth_;

                        Token_iterator token_iter_end;
                        while (token_iter_ != token_iter_end && token_iter_->type != Token_type::right_brace) {
                            compile_statement();
                        }
                        ensure_token_is(*token_iter_, Token_type::right_brace);
                        pop_local_scope(*token_iter_++);

                        break;
                    }

                    case Token_type::print: {
                        const auto print_token = *token_iter_++;

                        compile_assignment_precedence_expression();
                        ensure_token_is(*token_iter_++, Token_type::semicolon);
                        function_chunks_.back().emit<Opcode::print>(print_token);

                        break;
                    }

                    case Token_type::return_: {
                        const auto return_token = *token_iter_++;

                        if (token_iter_->type != Token_type::semicolon) {
                            compile_assignment_precedence_expression();
                        } else {
                            function_chunks_.back().emit<Opcode::nil>(return_token);
                        }
                        ensure_token_is(*token_iter_++, Token_type::semicolon);
                        function_chunks_.back().emit<Opcode::return_>(return_token);

                        break;
                    }

                    case Token_type::var: {
                        const auto var_token = *token_iter_++;

                        ensure_token_is(*token_iter_, Token_type::identifier);
                        const auto variable_name_token = *token_iter_++;

                        if (scope_depth_ > 0) {
                            const auto maybe_redeclared_iter = std::find_if(
                                tracked_locals_.crbegin(), tracked_locals_.crend(),
                                [&] (const auto& tracked_local) {
                                    return tracked_local.depth == scope_depth_ && tracked_local.name.lexeme == variable_name_token.lexeme;
                                }
                            );
                            if (maybe_redeclared_iter != tracked_locals_.crend()) {
                                std::ostringstream os;
                                os << "[Line " << variable_name_token.line << "] Error at \"" << variable_name_token.lexeme
                                    << "\": Variable with this name already declared in this scope.";
                                throw std::runtime_error{os.str()};
                            }

                            tracked_locals_.push_back({variable_name_token, scope_depth_, false});
                        }

                        if (token_iter_->type == Token_type::equal) {
                            ++token_iter_;
                            compile_assignment_precedence_expression();
                        } else {
                            function_chunks_.back().emit<Opcode::nil>(var_token);
                        }
                        ensure_token_is(*token_iter_++, Token_type::semicolon);

                        if (scope_depth_ == 0) {
                            function_chunks_.back().emit<Opcode::define_global>(variable_name_token, var_token);
                        } else {
                            // No bytecode to emit. The value is already on the stack,
                            // and the tracked local index will correspond to the stack position.
                            tracked_locals_.back().initialized = true;
                        }

                        break;
                    }

                    case Token_type::while_: {
                        const auto while_token = *token_iter_++;
                        const auto loop_begin_bytecode_index = function_chunks_.back().bytecode().size();

                        ensure_token_is(*token_iter_++, Token_type::left_paren);
                        compile_assignment_precedence_expression();
                        ensure_token_is(*token_iter_++, Token_type::right_paren);

                        auto to_end_jump_backpatch = function_chunks_.back().emit_jump_if_false(while_token);
                        function_chunks_.back().emit<Opcode::pop>(while_token);

                        compile_statement();

                        function_chunks_.back().emit_loop(loop_begin_bytecode_index, while_token);
                        to_end_jump_backpatch.to_next_opcode();
                        function_chunks_.back().emit<Opcode::pop>(while_token);

                        break;
                    }
                }
            }
    };
}

namespace motts { namespace lox {
    std::ostream& operator<<(std::ostream& os, const Opcode& opcode) {
        std::string name {([&] () {
            switch (opcode) {
                default:
                    throw std::logic_error{"Unexpected opcode."};

                #define X(name) \
                    case Opcode::name: \
                        return #name;
                MOTTS_LOX_OPCODE_NAMES
                #undef X
            }
        })()};

        // Names should print as uppercase without trailing underscores
        boost::trim_right_if(name, boost::is_any_of("_"));
        boost::to_upper(name);

        os << name;

        return os;
    }

    bool operator==(const Dynamic_type_value& lhs, const Dynamic_type_value& rhs) {
        return lhs.variant == rhs.variant;
    }

    std::ostream& operator<<(std::ostream& os, const Dynamic_type_value& value) {
        std::visit(Dynamic_type_print_visitor{os}, value.variant);
        return os;
    }

    Jump_backpatch::Jump_backpatch(Bytecode_vector& bytecode)
        : bytecode_ {bytecode},
          jump_begin_index_ {bytecode.size()}
    {
    }

    void Jump_backpatch::to_next_opcode() {
        const auto jump_distance = gsl::narrow<std::uint16_t>(bytecode_.size() - jump_begin_index_);
        const auto jump_distance_big_endian = boost::endian::native_to_big(jump_distance);
        reinterpret_cast<std::uint16_t&>(bytecode_.at(jump_begin_index_ - 2)) = jump_distance_big_endian;
    }

    template<Opcode opcode>
        void Chunk::emit(const Token& source_map_token) {
            bytecode_.push_back(gsl::narrow<std::uint8_t>(opcode));
            source_map_tokens_.push_back(source_map_token);
        }

    // Explicit instantiation of the opcodes allowed with this templated member function.
    // `emit<Opcode::constant>`, for example, should *not* be accepted through this template.
    template void Chunk::emit<Opcode::add>(const Token&);
    template void Chunk::emit<Opcode::divide>(const Token&);
    template void Chunk::emit<Opcode::false_>(const Token&);
    template void Chunk::emit<Opcode::equal>(const Token&);
    template void Chunk::emit<Opcode::greater>(const Token&);
    template void Chunk::emit<Opcode::less>(const Token&);
    template void Chunk::emit<Opcode::multiply>(const Token&);
    template void Chunk::emit<Opcode::negate>(const Token&);
    template void Chunk::emit<Opcode::nil>(const Token&);
    template void Chunk::emit<Opcode::not_>(const Token&);
    template void Chunk::emit<Opcode::pop>(const Token&);
    template void Chunk::emit<Opcode::print>(const Token&);
    template void Chunk::emit<Opcode::return_>(const Token&);
    template void Chunk::emit<Opcode::subtract>(const Token&);
    template void Chunk::emit<Opcode::true_>(const Token&);

    template<Opcode opcode>
        void Chunk::emit(const Token& variable_name, const Token& source_map_token) {
            const auto constant_index = insert_constant(Dynamic_type_value{std::string{variable_name.lexeme}});

            bytecode_.push_back(gsl::narrow<std::uint8_t>(opcode));
            bytecode_.push_back(constant_index);

            source_map_tokens_.push_back(source_map_token);
            source_map_tokens_.push_back(source_map_token);
        }

    template void Chunk::emit<Opcode::define_global>(const Token&, const Token&);
    template void Chunk::emit<Opcode::get_global>(const Token&, const Token&);
    template void Chunk::emit<Opcode::set_global>(const Token&, const Token&);

    template<Opcode opcode>
        void Chunk::emit(int local_stack_index, const Token& source_map_token) {
            bytecode_.push_back(gsl::narrow<std::uint8_t>(opcode));
            bytecode_.push_back(gsl::narrow<std::uint8_t>(local_stack_index));

            source_map_tokens_.push_back(source_map_token);
            source_map_tokens_.push_back(source_map_token);
        }

    template void Chunk::emit<Opcode::get_local>(int, const Token&);
    template void Chunk::emit<Opcode::set_local>(int, const Token&);

    void Chunk::emit_call(int arg_count, const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::call));
        bytecode_.push_back(gsl::narrow<std::uint8_t>(arg_count));

        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_constant(const Dynamic_type_value& constant_value, const Token& source_map_token) {
        const auto constant_index = insert_constant(constant_value);

        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::constant));
        bytecode_.push_back(constant_index);

        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);
    }

    Jump_backpatch Chunk::emit_jump_if_false(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::jump_if_false));
        bytecode_.push_back(0);
        bytecode_.push_back(0);

        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);

        return Jump_backpatch{bytecode_};
    }

    Jump_backpatch Chunk::emit_jump(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::jump));
        bytecode_.push_back(0);
        bytecode_.push_back(0);

        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);

        return Jump_backpatch{bytecode_};
    }

    void Chunk::emit_loop(Bytecode_vector::size_type loop_begin_bytecode_index, const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::loop));
        bytecode_.push_back(0);
        bytecode_.push_back(0);

        const auto jump_distance = gsl::narrow<std::uint16_t>(bytecode_.size() - loop_begin_bytecode_index);
        const auto jump_distance_big_endian = boost::endian::native_to_big(jump_distance);
        reinterpret_cast<std::uint16_t&>(*(bytecode_.end() - 2)) = jump_distance_big_endian;

        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);
    }

    decltype(Chunk::constants_)::size_type Chunk::insert_constant(const Dynamic_type_value& constant_value) {
        const auto maybe_constant_iter = std::find(constants_.cbegin(), constants_.cend(), constant_value);
        if (maybe_constant_iter != constants_.cend()) {
            const auto constant_index = maybe_constant_iter - constants_.cbegin();
            return gsl::narrow<decltype(constants_.size())>(constant_index);
        }

        const auto constant_index = constants_.size();
        constants_.push_back(constant_value);

        return constant_index;
    }

    std::ostream& operator<<(std::ostream& os, const Chunk& chunk) {
        const auto nested_function_chunck_iter = std::find_if(
            chunk.constants().cbegin(), chunk.constants().cend(),
            [&] (const auto& constant_dynamic_value) {
                return std::holds_alternative<Function*>(constant_dynamic_value.variant);
            }
        );
        if (nested_function_chunck_iter != chunk.constants().cend()) {
            os << "## main chunk\n";
        }

        os << "Constants:\n";
        for (auto constant_iter = chunk.constants().cbegin(); constant_iter != chunk.constants().cend(); ++constant_iter) {
            const auto index = constant_iter - chunk.constants().cbegin();
            os << std::setw(5) << index << " : " << *constant_iter << "\n";
        }

        os << "Bytecode:\n";
        for (auto bytecode_iter = chunk.bytecode().cbegin(); bytecode_iter != chunk.bytecode().cend(); ) {
            std::ostringstream line;

            const auto bytecode_index = bytecode_iter - chunk.bytecode().cbegin();
            const auto opcode = static_cast<Opcode>(*bytecode_iter);

            line << std::setw(5) << std::setfill(' ') << bytecode_index
                << " : "
                << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(opcode)
                << " ";

            switch (opcode) {
                default:
                    throw std::logic_error{"Unexpected opcode."};

                case Opcode::call:
                case Opcode::constant:
                case Opcode::define_global:
                case Opcode::get_global:
                case Opcode::get_local:
                case Opcode::set_global:
                case Opcode::set_local: {
                    line << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(*(bytecode_iter + 1))
                        << "    " << opcode << " [" << std::setbase(10) << static_cast<int>(*(bytecode_iter + 1)) << "]";
                    bytecode_iter += 2;
                    break;
                }

                case Opcode::jump_if_false:
                case Opcode::jump:
                case Opcode::loop: {
                    const auto jump_distance = boost::endian::big_to_native(reinterpret_cast<const std::uint16_t&>(*(bytecode_iter + 1)));
                    const auto jump_target = bytecode_index + 3 + (opcode == Opcode::loop ? -1 : 1) * jump_distance;

                    line << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(*(bytecode_iter + 1)) << ' '
                        << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(*(bytecode_iter + 2)) << ' '
                        << opcode << ' '
                        << (opcode == Opcode::loop ? '-' : '+') << std::setbase(10) << jump_distance
                        << " -> " << jump_target;

                    bytecode_iter += 3;

                    break;
                }

                case Opcode::add:
                case Opcode::divide:
                case Opcode::equal:
                case Opcode::false_:
                case Opcode::greater:
                case Opcode::less:
                case Opcode::multiply:
                case Opcode::negate:
                case Opcode::nil:
                case Opcode::not_:
                case Opcode::pop:
                case Opcode::print:
                case Opcode::return_:
                case Opcode::subtract:
                case Opcode::true_: {
                    line << std::setw(6) << std::setfill(' ') << " " << opcode;
                    bytecode_iter += 1;
                    break;
                }
            }

            const auto& source_map_token = chunk.source_map_tokens().at(bytecode_index);
            os << std::setw(40) << std::setfill(' ') << std::left << line.str()
                << " ; " << source_map_token.lexeme << " @ " << source_map_token.line << "\n";
        }

        for (const auto& constant_dynamic_value : chunk.constants()) {
            if (std::holds_alternative<Function*>(constant_dynamic_value.variant)) {
                os << "## " << constant_dynamic_value << " chunk\n"
                    << std::get<Function*>(constant_dynamic_value.variant)->chunk;
            }
        }

        return os;
    }

    Chunk compile(std::string_view source) {
        return Compiler{source}.compile();
    }
}}
