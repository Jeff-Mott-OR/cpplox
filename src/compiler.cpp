#include "compiler.hpp"

#include <cstddef>
#include <iomanip>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <gsl/gsl>

#include "scanner.hpp"

namespace {
    struct Print_visitor {
        std::ostream& os;
        Print_visitor(std::ostream& os_arg)
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

    std::ostream& operator<<(std::ostream& os, const Dynamic_type_value& value) {
        std::visit(Print_visitor{os}, value.variant);
        return os;
    }

    void Chunk::emit_add(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::add));
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_constant(const Dynamic_type_value& constant_value, const Token& source_map_token) {
        const auto constant_index = constants_.size();
        constants_.push_back(constant_value);

        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::constant));
        bytecode_.push_back(constant_index);

        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_divide(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::divide));
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_equal(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::equal));
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_greater(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::greater));
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_false(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::false_));
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_less(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::less));
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_multiply(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::multiply));
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_negate(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::negate));
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_nil(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::nil));
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_not(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::not_));
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_pop(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::pop));
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_print(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::print));
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_subtract(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::subtract));
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_true(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::true_));
        source_map_tokens_.push_back(source_map_token);
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

            const auto index = bytecode_iter - chunk.bytecode().cbegin();
            const auto opcode = static_cast<Opcode>(*bytecode_iter);

            line << std::setw(5) << std::setfill(' ') << index
                << " : "
                << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(opcode)
                << " ";

            switch (opcode) {
                default:
                    throw std::logic_error{"Unexpected opcode."};

                case Opcode::constant:
                    line << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(*(bytecode_iter + 1))
                        << " " << opcode << " [" << std::setbase(10) << static_cast<int>(*(bytecode_iter + 1)) << "]";
                    bytecode_iter += 2;

                    break;

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
                case Opcode::true_:
                    line << std::setw(3) << std::setfill(' ') << " " << opcode;
                    bytecode_iter += 1;

                    break;
            }

            const auto source_map_token = chunk.source_map_tokens().at(index);
            os << std::setw(27) << std::setfill(' ') << std::left << line.str()
                << " ; " << source_map_token.lexeme << " @ " << source_map_token.line << "\n";
        }

        return os;
    }

    /* no export */ static void ensure_token_is(const Token& token, const Token_type& expected) {
        if (token.type != expected) {
            std::ostringstream os;
            os << "[Line " << token.line << "] Error: Expected " << expected << ", at \"" << token.lexeme << "\".";
            throw std::runtime_error{os.str()};
        }
    }

    void compile_equality_precedence_expression(Chunk&, Token_iterator&);

    void compile_primary_expression(Chunk& chunk, Token_iterator& token_iter) {
        switch (token_iter->type) {
            default:
                throw std::runtime_error{
                    "[Line " + std::to_string(token_iter->line) + "] Error: Unexpected token \"" + std::string{token_iter->lexeme} + "\"."
                };

            case Token_type::false_:
                chunk.emit_false(*token_iter);
                break;

            case Token_type::left_paren:
                ++token_iter;
                compile_equality_precedence_expression(chunk, token_iter);
                ensure_token_is(*token_iter, Token_type::right_paren);
                break;

            case Token_type::nil:
                chunk.emit_nil(*token_iter);
                break;

            case Token_type::number: {
                const auto number_value = boost::lexical_cast<double>(token_iter->lexeme);
                chunk.emit_constant(Dynamic_type_value{number_value}, *token_iter);
                break;
            }

            case Token_type::string: {
                auto string_value = std::string{token_iter->lexeme.cbegin() + 1, token_iter->lexeme.cend() - 1};
                chunk.emit_constant(Dynamic_type_value{std::move(string_value)}, *token_iter);
                break;
            }

            case Token_type::true_:
                chunk.emit_true(*token_iter);
                break;
        }

        ++token_iter;
    }

    void compile_unary_precedence_expression(Chunk& chunk, Token_iterator& token_iter) {
        if (token_iter->type == Token_type::minus || token_iter->type == Token_type::bang) {
            const auto unary_op_token = *token_iter;
            ++token_iter;

            // Right expression
            compile_unary_precedence_expression(chunk, token_iter);

            switch (unary_op_token.type) {
                default:
                    throw std::logic_error{"Unreachable"};

                case Token_type::minus:
                    chunk.emit_negate(unary_op_token);
                    break;

                case Token_type::bang:
                    chunk.emit_not(unary_op_token);
                    break;
            }

            return;
        }

        compile_primary_expression(chunk, token_iter);
    }

    void compile_multiplication_precedence_expression(Chunk& chunk, Token_iterator& token_iter) {
        // Left expression
        compile_unary_precedence_expression(chunk, token_iter);

        while (token_iter->type == Token_type::star || token_iter->type == Token_type::slash) {
            const auto binary_op_token = *token_iter;
            ++token_iter;

            // Right expression
            compile_unary_precedence_expression(chunk, token_iter);

            switch (binary_op_token.type) {
                default:
                    throw std::logic_error{"Unreachable"};

                case Token_type::star:
                    chunk.emit_multiply(binary_op_token);
                    break;

                case Token_type::slash:
                    chunk.emit_divide(binary_op_token);
                    break;
            }
        }
    }

    void compile_addition_precedence_expression(Chunk& chunk, Token_iterator& token_iter) {
        // Left expression
        compile_multiplication_precedence_expression(chunk, token_iter);

        while (token_iter->type == Token_type::plus || token_iter->type == Token_type::minus) {
            const auto binary_op_token = *token_iter;
            ++token_iter;

            // Right expression
            compile_multiplication_precedence_expression(chunk, token_iter);

            switch (binary_op_token.type) {
                default:
                    throw std::logic_error{"Unreachable"};

                case Token_type::plus:
                    chunk.emit_add(binary_op_token);
                    break;

                case Token_type::minus:
                    chunk.emit_subtract(binary_op_token);
                    break;
            }
        }
    }

    void compile_comparison_precedence_expression(Chunk& chunk, Token_iterator& token_iter) {
        // Left expression
        compile_addition_precedence_expression(chunk, token_iter);

        while (
            token_iter->type == Token_type::less || token_iter->type == Token_type::less_equal ||
            token_iter->type == Token_type::greater || token_iter->type == Token_type::greater_equal
        ) {
            const auto comparison_token = *token_iter;
            ++token_iter;

            // Right expression
            compile_addition_precedence_expression(chunk, token_iter);

            switch (comparison_token.type) {
                default:
                    throw std::logic_error{"Unreachable"};

                case Token_type::less:
                    chunk.emit_less(comparison_token);
                    break;

                case Token_type::less_equal:
                    chunk.emit_greater(comparison_token);
                    chunk.emit_not(comparison_token);
                    break;

                case Token_type::greater:
                    chunk.emit_greater(comparison_token);
                    break;

                case Token_type::greater_equal:
                    chunk.emit_less(comparison_token);
                    chunk.emit_not(comparison_token);
                    break;
            }
        }
    }

    void compile_equality_precedence_expression(Chunk& chunk, Token_iterator& token_iter) {
        // Left expression
        compile_comparison_precedence_expression(chunk, token_iter);

        while (token_iter->type == Token_type::equal_equal || token_iter->type == Token_type::bang_equal) {
            const auto equality_token = *token_iter;
            ++token_iter;

            // Right expression
            compile_comparison_precedence_expression(chunk, token_iter);

            switch (equality_token.type) {
                default:
                    throw std::logic_error{"Unreachable"};

                case Token_type::equal_equal:
                    chunk.emit_equal(equality_token);
                    break;

                case Token_type::bang_equal:
                    chunk.emit_equal(equality_token);
                    chunk.emit_not(equality_token);
                    break;
            }
        }
    }

    Chunk compile(std::string_view source) {
        Chunk chunk;

        Token_iterator token_iter_end;
        for (Token_iterator token_iter {source}; token_iter != token_iter_end; ) {
            if (token_iter->type == Token_type::print) {
                const auto print_token = *token_iter;
                ++token_iter;

                compile_equality_precedence_expression(chunk, token_iter);
                chunk.emit_print(print_token);
                ensure_token_is(*token_iter, Token_type::semicolon);
                ++token_iter;

                continue;
            }

            compile_equality_precedence_expression(chunk, token_iter);
            chunk.emit_pop(*token_iter);
            ensure_token_is(*token_iter, Token_type::semicolon);
            ++token_iter;
        }

        return chunk;
    }
}}
