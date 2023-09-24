#include "compiler.hpp"

#include <iomanip>

#include <boost/algorithm/string.hpp>

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
        boost::to_upper(name);

        os << name;

        return os;
    }

    std::ostream& operator<<(std::ostream& os, const Chunk& chunk) {
        os << "Constants:\n";

        for (auto constant_iter = chunk.constants.cbegin(); constant_iter != chunk.constants.cend(); ++constant_iter) {
            const auto index = constant_iter - chunk.constants.cbegin();
            os << std::setw(5) << index << " : " << *constant_iter << "\n";
        }

        os << "Bytecode:\n";

        for (auto bytecode_iter = chunk.bytecode.cbegin(); bytecode_iter != chunk.bytecode.cend(); ) {
            const auto index = bytecode_iter - chunk.bytecode.cbegin();
            const auto opcode = static_cast<Opcode>(*bytecode_iter);

            os << std::setw(5) << std::setfill(' ') << index
                << " : "
                << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(opcode)
                << " ";

            switch (opcode) {
                default:
                    throw std::logic_error{"Unexpected opcode."};

                case Opcode::constant:
                    if (chunk.bytecode.size() - index < 2) {
                        throw std::runtime_error{"Insufficient byte for opcode."};
                    }
                    os << std::setw(2) << std::setfill('0') << std::setbase(16) << static_cast<int>(*(bytecode_iter + 1))
                        << " " << opcode << " [" << static_cast<int>(*(bytecode_iter + 1)) << "]";
                    bytecode_iter += 2;

                    break;

                case Opcode::nil:
                    os << std::setw(3) << std::setfill(' ') << " " << opcode;
                    bytecode_iter += 1;

                    break;
            }

            const auto source_map_token = chunk.source_map_tokens.at(index);
            os << " ; " << source_map_token.lexeme << " @ " << source_map_token.line << "\n";
        }

        return os;
    }
}}
