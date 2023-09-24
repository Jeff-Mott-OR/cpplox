#include "vm.hpp"

#include <iomanip>
#include <vector>

#include <boost/endian/conversion.hpp>

namespace {
    struct Truthy_visitor {
        auto operator()(std::nullptr_t) const {
            return false;
        }

        auto operator()(bool value) const {
            return value;
        }

        template<typename T>
            auto operator()(const T&) const {
               return true;
            }
    };
}

namespace motts { namespace lox {
    void run(const Chunk& chunk, std::ostream& os, bool debug) {
        if (debug) {
            os << "# Running chunk:\n" << chunk << "\n";
        }

        std::vector<Dynamic_type_value> stack;

        for (auto bytecode_iter = chunk.bytecode().cbegin(); bytecode_iter != chunk.bytecode().cend(); ) {
            const auto index = bytecode_iter - chunk.bytecode().cbegin();
            const auto& source_map_token = chunk.source_map_tokens().at(index);

            const auto& opcode = static_cast<Opcode>(*bytecode_iter);
            switch (opcode) {
                default: {
                    std::ostringstream os;
                    os << "[Line " << source_map_token.line << "] Error: Unexpected opcode " << opcode
                        << ", generated from source \"" << source_map_token.lexeme << "\".";
                    throw std::runtime_error{os.str()};
                }

                case Opcode::add: {
                    const auto& b = *(stack.cend() - 1);
                    const auto& a = *(stack.cend() - 2);

                    if (std::holds_alternative<double>(a.variant) && std::holds_alternative<double>(b.variant))
                    {
                        const auto result = std::get<double>(a.variant) + std::get<double>(b.variant);
                        stack.erase(stack.cend() - 2, stack.cend());
                        stack.push_back(Dynamic_type_value{result});
                    }
                    else if (std::holds_alternative<std::string>(a.variant) && std::holds_alternative<std::string>(b.variant))
                    {
                        auto result = std::get<std::string>(a.variant) + std::get<std::string>(b.variant);
                        stack.erase(stack.cend() - 2, stack.cend());
                        stack.push_back(Dynamic_type_value{std::move(result)});
                    }
                    else
                    {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be two numbers or two strings."
                        };
                    }

                    ++bytecode_iter;
                    break;
                }

                case Opcode::constant: {
                    const auto constant_index = *(bytecode_iter + 1);
                    stack.push_back(chunk.constants().at(constant_index));
                    bytecode_iter += 2;
                    break;
                }

                case Opcode::divide: {
                    const auto& b = *(stack.cend() - 1);
                    const auto& a = *(stack.cend() - 2);

                    if (! std::holds_alternative<double>(a.variant) || ! std::holds_alternative<double>(b.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(a.variant) / std::get<double>(b.variant);
                    stack.erase(stack.cend() - 2, stack.cend());
                    stack.push_back(Dynamic_type_value{result});

                    ++bytecode_iter;
                    break;
                }

                case Opcode::equal: {
                    const auto& b = *(stack.cend() - 1);
                    const auto& a = *(stack.cend() - 2);

                    if (! std::holds_alternative<double>(a.variant) || ! std::holds_alternative<double>(b.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(a.variant) == std::get<double>(b.variant);
                    stack.erase(stack.cend() - 2, stack.cend());
                    stack.push_back(Dynamic_type_value{result});

                    ++bytecode_iter;
                    break;
                }

                case Opcode::false_: {
                    stack.push_back(Dynamic_type_value{false});
                    ++bytecode_iter;
                    break;
                }

                case Opcode::greater: {
                    const auto& b = *(stack.cend() - 1);
                    const auto& a = *(stack.cend() - 2);

                    if (! std::holds_alternative<double>(a.variant) || ! std::holds_alternative<double>(b.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(a.variant) > std::get<double>(b.variant);
                    stack.erase(stack.cend() - 2, stack.cend());
                    stack.push_back(Dynamic_type_value{result});

                    ++bytecode_iter;
                    break;
                }

                case Opcode::jump: {
                    const auto jump_distance_big_endian = reinterpret_cast<const std::uint16_t&>(*(bytecode_iter + 1));
                    const auto jump_distance = boost::endian::big_to_native(jump_distance_big_endian);
                    bytecode_iter += 3 + jump_distance;

                    break;
                }

                case Opcode::jump_if_false: {
                    const auto jump_distance_big_endian = reinterpret_cast<const std::uint16_t&>(*(bytecode_iter + 1));
                    const auto jump_distance = boost::endian::big_to_native(jump_distance_big_endian);

                    bytecode_iter += 3;
                    if (! std::visit(Truthy_visitor{}, stack.back().variant)) {
                        bytecode_iter += jump_distance;
                    }

                    break;
                }

                case Opcode::less: {
                    const auto& b = *(stack.cend() - 1);
                    const auto& a = *(stack.cend() - 2);

                    if (! std::holds_alternative<double>(a.variant) || ! std::holds_alternative<double>(b.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(a.variant) < std::get<double>(b.variant);
                    stack.erase(stack.cend() - 2, stack.cend());
                    stack.push_back(Dynamic_type_value{result});

                    ++bytecode_iter;
                    break;
                }

                case Opcode::multiply: {
                    const auto& b = *(stack.cend() - 1);
                    const auto& a = *(stack.cend() - 2);

                    if (! std::holds_alternative<double>(a.variant) || ! std::holds_alternative<double>(b.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(a.variant) * std::get<double>(b.variant);
                    stack.erase(stack.cend() - 2, stack.cend());
                    stack.push_back(Dynamic_type_value{result});

                    ++bytecode_iter;
                    break;
                }

                case Opcode::negate: {
                    if (! std::holds_alternative<double>(stack.back().variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operand must be a number."
                        };
                    }

                    const auto negated_value = - std::get<double>(stack.back().variant);

                    stack.pop_back();
                    stack.push_back(Dynamic_type_value{negated_value});

                    ++bytecode_iter;
                    break;
                }

                case Opcode::nil: {
                    stack.push_back(Dynamic_type_value{nullptr});
                    ++bytecode_iter;
                    break;
                }

                case Opcode::not_: {
                    const auto negated_value = ! std::visit(Truthy_visitor{}, stack.back().variant);

                    stack.pop_back();
                    stack.push_back(Dynamic_type_value{negated_value});

                    ++bytecode_iter;
                    break;
                }

                case Opcode::pop: {
                    stack.pop_back();
                    ++bytecode_iter;
                    break;
                }

                case Opcode::print: {
                    os << stack.back() << "\n";
                    stack.pop_back();
                    ++bytecode_iter;
                    break;
                }

                case Opcode::subtract: {
                    const auto& b = *(stack.cend() - 1);
                    const auto& a = *(stack.cend() - 2);

                    if (! std::holds_alternative<double>(a.variant) || ! std::holds_alternative<double>(b.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(a.variant) - std::get<double>(b.variant);
                    stack.erase(stack.cend() - 2, stack.cend());
                    stack.push_back(Dynamic_type_value{result});

                    ++bytecode_iter;
                    break;
                }

                case Opcode::true_: {
                    stack.push_back(Dynamic_type_value{true});
                    ++bytecode_iter;
                    break;
                }
            }

            if (debug) {
                os << "Stack:\n";
                for (auto stack_iter = stack.crbegin(); stack_iter != stack.crend(); ++stack_iter) {
                    const auto index = stack_iter.base() - 1 - stack.cbegin();
                    os << std::setw(5) << std::setfill(' ') << std::right << index << " : " << *stack_iter << "\n";
                }
                os << "\n";
            }
        }
    }
}}
