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
        bool initialized {false};
    };

    // There's no invariant being maintained here.
    // This exists primarily to avoid lots of manual argument passing.
    class Compiler {
        Chunk chunk_;
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

                return std::move(chunk_);
            }

        private:
            void compile_primary_expression() {
                switch (token_iter_->type) {
                    default: {
                        std::ostringstream os;
                        os << "[Line " << token_iter_->line << "] Error: Unexpected token \"" << token_iter_->lexeme << "\".";
                        throw std::runtime_error{os.str()};
                    }

                    case Token_type::false_: {
                        chunk_.emit<Opcode::false_>(*token_iter_);
                        break;
                    }

                    case Token_type::identifier: {
                        const auto maybe_local_iter = std::find_if(
                            tracked_locals_.crbegin(), tracked_locals_.crend(),
                            [&] (const auto& tracked_local) {
                                return tracked_local.name.lexeme == token_iter_->lexeme;
                            }
                        );

                        if (maybe_local_iter != tracked_locals_.crend()) {
                            const auto local_iter = maybe_local_iter.base() - 1;

                            if (! local_iter->initialized) {
                                std::ostringstream os;
                                os << "[Line " << token_iter_->line << "] Error at \"" << token_iter_->lexeme
                                    << "\": Cannot read local variable in its own initializer.";
                                throw std::runtime_error{os.str()};
                            }

                            const auto tracked_local_stack_index = local_iter - tracked_locals_.cbegin();
                            chunk_.emit<Opcode::get_local>(tracked_local_stack_index, *token_iter_);
                        } else {
                            chunk_.emit<Opcode::get_global>(*token_iter_, *token_iter_);
                        }

                        break;
                    }

                    case Token_type::left_paren: {
                        ++token_iter_;
                        compile_assignment_precedence_expression();
                        ensure_token_is(*token_iter_, Token_type::right_paren);

                        break;
                    }

                    case Token_type::nil: {
                        chunk_.emit<Opcode::nil>(*token_iter_);
                        break;
                    }

                    case Token_type::number: {
                        const auto number_value = boost::lexical_cast<double>(token_iter_->lexeme);
                        chunk_.emit_constant(Dynamic_type_value{number_value}, *token_iter_);

                        break;
                    }

                    case Token_type::string: {
                        auto string_value = std::string{token_iter_->lexeme.cbegin() + 1, token_iter_->lexeme.cend() - 1};
                        chunk_.emit_constant(Dynamic_type_value{std::move(string_value)}, *token_iter_);

                        break;
                    }

                    case Token_type::true_: {
                        chunk_.emit<Opcode::true_>(*token_iter_);
                        break;
                    }
                }

                ++token_iter_;
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
                            chunk_.emit<Opcode::negate>(unary_op_token);
                            break;

                        case Token_type::bang:
                            chunk_.emit<Opcode::not_>(unary_op_token);
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
                            chunk_.emit<Opcode::multiply>(binary_op_token);
                            break;

                        case Token_type::slash:
                            chunk_.emit<Opcode::divide>(binary_op_token);
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
                            chunk_.emit<Opcode::add>(binary_op_token);
                            break;

                        case Token_type::minus:
                            chunk_.emit<Opcode::subtract>(binary_op_token);
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
                            chunk_.emit<Opcode::less>(comparison_token);
                            break;

                        case Token_type::less_equal:
                            chunk_.emit<Opcode::greater>(comparison_token);
                            chunk_.emit<Opcode::not_>(comparison_token);
                            break;

                        case Token_type::greater:
                            chunk_.emit<Opcode::greater>(comparison_token);
                            break;

                        case Token_type::greater_equal:
                            chunk_.emit<Opcode::less>(comparison_token);
                            chunk_.emit<Opcode::not_>(comparison_token);
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
                            chunk_.emit<Opcode::equal>(equality_token);
                            break;

                        case Token_type::bang_equal:
                            chunk_.emit<Opcode::equal>(equality_token);
                            chunk_.emit<Opcode::not_>(equality_token);
                            break;
                    }
                }
            }

            void compile_and_precedence_expression() {
                // Left expression
                compile_equality_precedence_expression();

                while (token_iter_->type == Token_type::and_) {
                    auto short_circuit_jump_backpatch = chunk_.emit_jump_if_false(*token_iter_);
                    // If the LHS was true, then the expression now depends solely on the RHS, and we can discard the LHS
                    chunk_.emit<Opcode::pop>(*token_iter_++);

                    // Right expression
                    compile_equality_precedence_expression();

                    short_circuit_jump_backpatch.to_next_opcode();
                }
            }

            void compile_or_precedence_expression() {
                // Left expression
                compile_and_precedence_expression();

                while (token_iter_->type == Token_type::or_) {
                    auto to_rhs_jump_backpatch = chunk_.emit_jump_if_false(*token_iter_);
                    auto to_end_jump_backpatch = chunk_.emit_jump(*token_iter_);

                    to_rhs_jump_backpatch.to_next_opcode();
                    // If the LHS was false, then the expression now depends solely on the RHS, and we can discard the LHS
                    chunk_.emit<Opcode::pop>(*token_iter_++);

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
                            chunk_.emit<Opcode::set_local>(tracked_local_stack_index, variable_name_token);
                        } else {
                            chunk_.emit<Opcode::set_global>(variable_name_token, assign_token);
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
                chunk_.emit<Opcode::pop>(*token_iter_++);
            }

            void compile_statement() {
                if (token_iter_->type == Token_type::left_brace) {
                    ++token_iter_;

                    ++scope_depth_;
                    const auto _ = gsl::finally([&] () {
                        while (! tracked_locals_.empty() && tracked_locals_.back().depth == scope_depth_) {
                            tracked_locals_.pop_back();
                        }
                        --scope_depth_;
                    });

                    Token_iterator token_iter_end;
                    for ( ; token_iter_ != token_iter_end && token_iter_->type != Token_type::right_brace; ) {
                        compile_statement();
                    }
                    ensure_token_is(*token_iter_++, Token_type::right_brace);

                    return;
                }

                if (token_iter_->type == Token_type::if_) {
                    const auto if_token = *token_iter_++;

                    ensure_token_is(*token_iter_++, Token_type::left_paren);
                    compile_assignment_precedence_expression();
                    ensure_token_is(*token_iter_++, Token_type::right_paren);

                    auto to_else_or_end_jump_backpatch = chunk_.emit_jump_if_false(if_token);
                    chunk_.emit<Opcode::pop>(if_token);
                    compile_statement();

                    if (token_iter_->type == Token_type::else_) {
                        const auto else_token = *token_iter_++;
                        auto to_end_jump_backpatch = chunk_.emit_jump(else_token);

                        to_else_or_end_jump_backpatch.to_next_opcode();
                        chunk_.emit<Opcode::pop>(if_token);
                        compile_statement();

                        to_end_jump_backpatch.to_next_opcode();
                    } else {
                        to_else_or_end_jump_backpatch.to_next_opcode();
                        chunk_.emit<Opcode::pop>(if_token);
                    }

                    return;
                }

                if (token_iter_->type == Token_type::print) {
                    const auto print_token = *token_iter_++;

                    compile_assignment_precedence_expression();
                    ensure_token_is(*token_iter_++, Token_type::semicolon);
                    chunk_.emit<Opcode::print>(print_token);

                    return;
                }

                if (token_iter_->type == Token_type::while_) {
                    const auto while_token = *token_iter_++;
                    const auto loop_begin_bytecode_index = chunk_.bytecode().size();

                    ensure_token_is(*token_iter_++, Token_type::left_paren);
                    compile_assignment_precedence_expression();
                    ensure_token_is(*token_iter_++, Token_type::right_paren);

                    auto to_end_jump_backpatch = chunk_.emit_jump_if_false(while_token);
                    chunk_.emit<Opcode::pop>(while_token);

                    compile_statement();

                    chunk_.emit_loop(loop_begin_bytecode_index, while_token);
                    to_end_jump_backpatch.to_next_opcode();
                    chunk_.emit<Opcode::pop>(while_token);

                    return;
                }

                if (token_iter_->type == Token_type::var) {
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

                        tracked_locals_.push_back({variable_name_token, scope_depth_});
                    }

                    if (token_iter_->type == Token_type::equal) {
                        ++token_iter_;
                        compile_assignment_precedence_expression();
                    } else {
                        chunk_.emit<Opcode::nil>(var_token);
                    }
                    ensure_token_is(*token_iter_++, Token_type::semicolon);

                    if (scope_depth_ == 0) {
                        chunk_.emit<Opcode::define_global>(variable_name_token, var_token);
                    } else {
                        // No bytecode to emit. The value is already on the stack,
                        // and the tracked local index will correspond to the stack position.
                        tracked_locals_.back().initialized = true;
                    }

                    return;
                }

                // Else
                compile_expression_statement();
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

        return os;
    }

    Chunk compile(std::string_view source) {
        return Compiler{source}.compile();
    }
}}
