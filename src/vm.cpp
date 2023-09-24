#include "vm.hpp"

#include <iomanip>
#include <vector>

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
                default:
                    throw std::runtime_error{
                        "[Line " + std::to_string(source_map_token.line) + "] Error: Unexpected opcode \"" + std::string{source_map_token.lexeme} + "\"."
                    };

                case Opcode::add: {
                    const auto b = (stack.cend() - 1)->variant;
                    const auto a = (stack.cend() - 2)->variant;
                    stack.erase(stack.cend() - 2, stack.cend());

                    if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b))
                    {
                        stack.push_back(Dynamic_type_value{std::get<double>(a) + std::get<double>(b)});
                    }
                    else if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b))
                    {
                        stack.push_back(Dynamic_type_value{std::get<std::string>(a) + std::get<std::string>(b)});
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
                    const auto b = (stack.cend() - 1)->variant;
                    const auto a = (stack.cend() - 2)->variant;
                    stack.erase(stack.cend() - 2, stack.cend());

                    stack.push_back(Dynamic_type_value{std::get<double>(a) / std::get<double>(b)});

                    ++bytecode_iter;
                    break;
                }

                case Opcode::false_:
                    stack.push_back(Dynamic_type_value{false});
                    ++bytecode_iter;
                    break;

                case Opcode::multiply: {
                    const auto b = (stack.cend() - 1)->variant;
                    const auto a = (stack.cend() - 2)->variant;
                    stack.erase(stack.cend() - 2, stack.cend());

                    stack.push_back(Dynamic_type_value{std::get<double>(a) * std::get<double>(b)});

                    ++bytecode_iter;
                    break;
                }

                case Opcode::nil:
                    stack.push_back(Dynamic_type_value{nullptr});
                    ++bytecode_iter;
                    break;

                case Opcode::print:
                    os << stack.back() << "\n";
                    stack.pop_back();
                    ++bytecode_iter;
                    break;

                case Opcode::subtract: {
                    const auto b = (stack.cend() - 1)->variant;
                    const auto a = (stack.cend() - 2)->variant;
                    stack.erase(stack.cend() - 2, stack.cend());

                    stack.push_back(Dynamic_type_value{std::get<double>(a) - std::get<double>(b)});

                    ++bytecode_iter;
                    break;
                }

                case Opcode::true_:
                    stack.push_back(Dynamic_type_value{true});
                    ++bytecode_iter;
                    break;
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
