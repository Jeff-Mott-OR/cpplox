#include "compiler.hpp"

#include <iomanip>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <gsl/gsl>

#include "scanner.hpp"

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

    struct Print_visitor {
        std::ostream& os;
        Print_visitor(std::ostream& os_arg)
            : os {os_arg}
        {
        }

        template<typename T>
            auto operator()(const T& value) {
                os << value;
            }
    };

    std::ostream& operator<<(std::ostream& os, const Dynamic_type_value& value) {
        std::visit(Print_visitor{os}, value.variant);
        return os;
    }

    const decltype(Chunk::constants_)& Chunk::constants() const {
        return constants_;
    }

    const decltype(Chunk::bytecode_)& Chunk::bytecode() const {
        return bytecode_;
    }

    const decltype(Chunk::source_map_tokens_)& Chunk::source_map_tokens() const {
        return source_map_tokens_;
    }

    void Chunk::emit_constant(const Dynamic_type_value& constant_value, const Token& source_map_token) {
        const auto constant_index = constants_.size();
        constants_.push_back(constant_value);

        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::constant));
        bytecode_.push_back(constant_index);

        source_map_tokens_.push_back(source_map_token);
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_false(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::false_));
        source_map_tokens_.push_back(source_map_token);
    }

    void Chunk::emit_nil(const Token& source_map_token) {
        bytecode_.push_back(gsl::narrow<std::uint8_t>(Opcode::nil));
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
                        << " " << opcode << " [" << static_cast<int>(*(bytecode_iter + 1)) << "]";
                    bytecode_iter += 2;

                    break;

                case Opcode::false_:
                case Opcode::nil:
                case Opcode::true_:
                    line << std::setw(3) << std::setfill(' ') << " " << opcode;
                    bytecode_iter += 1;

                    break;
            }

            const auto source_map_token = chunk.source_map_tokens().at(index);
            os << std::setw(26) << std::setfill(' ') << std::left << line.str()
                << " ; " << source_map_token.lexeme << " @ " << source_map_token.line << "\n";
        }

        return os;
    }

    Chunk compile(std::string_view source) {
        Chunk chunk;

        Token_iterator token_iter_end;
        for (Token_iterator token_iter {source}; token_iter != token_iter_end; ++token_iter) {
            switch (token_iter->type) {
                default:
                    throw std::runtime_error{
                        "[Line " + std::to_string(token_iter->line) + "] Error: Unexpected token \"" + std::string{token_iter->lexeme} + "\"."
                    };

                case Token_type::false_:
                    chunk.emit_false(*token_iter);
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
        }

        return chunk;
    }
}}
