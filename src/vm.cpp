#include "vm.hpp"

#include <iomanip>

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
        VM vm;
        vm.run(chunk, os, debug);
    }

    void VM::run(const Chunk& chunk, std::ostream& os, bool debug) {
        if (debug) {
            os << "# Running chunk:\n" << chunk << "\n";
        }

        for (auto bytecode_iter = chunk.bytecode().cbegin(); bytecode_iter != chunk.bytecode().cend(); ) {
            const auto bytecode_index = bytecode_iter - chunk.bytecode().cbegin();
            const auto& source_map_token = chunk.source_map_tokens().at(bytecode_index);

            const auto& opcode = static_cast<Opcode>(*bytecode_iter);
            switch (opcode) {
                default: {
                    std::ostringstream os;
                    os << "[Line " << source_map_token.line << "] Error: Unexpected opcode " << opcode
                        << ", generated from source \"" << source_map_token.lexeme << "\".";
                    throw std::runtime_error{os.str()};
                }

                case Opcode::add: {
                    const auto& b = *(stack_.cend() - 1);
                    const auto& a = *(stack_.cend() - 2);

                    if (std::holds_alternative<double>(a.variant) && std::holds_alternative<double>(b.variant))
                    {
                        const auto result = std::get<double>(a.variant) + std::get<double>(b.variant);
                        stack_.erase(stack_.cend() - 2, stack_.cend());
                        stack_.push_back(Dynamic_type_value{result});
                    }
                    else if (std::holds_alternative<std::string>(a.variant) && std::holds_alternative<std::string>(b.variant))
                    {
                        auto result = std::get<std::string>(a.variant) + std::get<std::string>(b.variant);
                        stack_.erase(stack_.cend() - 2, stack_.cend());
                        stack_.push_back(Dynamic_type_value{std::move(result)});
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
                    stack_.push_back(chunk.constants().at(constant_index));

                    bytecode_iter += 2;

                    break;
                }

                case Opcode::divide: {
                    const auto& b = *(stack_.cend() - 1);
                    const auto& a = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(a.variant) || ! std::holds_alternative<double>(b.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(a.variant) / std::get<double>(b.variant);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(Dynamic_type_value{result});

                    ++bytecode_iter;

                    break;
                }

                case Opcode::equal: {
                    const auto& b = *(stack_.cend() - 1);
                    const auto& a = *(stack_.cend() - 2);

                    const auto result = a.variant == b.variant;
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(Dynamic_type_value{result});

                    ++bytecode_iter;

                    break;
                }

                case Opcode::false_: {
                    stack_.push_back(Dynamic_type_value{false});

                    ++bytecode_iter;

                    break;
                }

                case Opcode::define_global: {
                    const auto variable_name_constant_index = *(bytecode_iter + 1);
                    const auto& variable_name = std::get<std::string>(chunk.constants().at(variable_name_constant_index).variant);

                    globals_[variable_name] = stack_.back();
                    stack_.pop_back();

                    bytecode_iter += 2;

                    break;
                }

                case Opcode::get_global: {
                    const auto variable_name_constant_index = *(bytecode_iter + 1);
                    const auto& variable_name = std::get<std::string>(chunk.constants().at(variable_name_constant_index).variant);

                    const auto global_iter = globals_.find(variable_name);
                    if (global_iter == globals_.end()) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Undefined variable '" + variable_name + "'."
                        };
                    }
                    stack_.push_back(global_iter->second);

                    bytecode_iter += 2;

                    break;
                }

                case Opcode::greater: {
                    const auto& b = *(stack_.cend() - 1);
                    const auto& a = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(a.variant) || ! std::holds_alternative<double>(b.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(a.variant) > std::get<double>(b.variant);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(Dynamic_type_value{result});

                    ++bytecode_iter;

                    break;
                }

                case Opcode::jump:
                case Opcode::jump_if_false:
                case Opcode::loop: {
                    const auto jump_distance_big_endian = reinterpret_cast<const std::uint16_t&>(*(bytecode_iter + 1));
                    const auto jump_distance = boost::endian::big_to_native(jump_distance_big_endian);

                    bytecode_iter += 3;

                    switch (opcode) {
                        default:
                            throw std::logic_error{"Unreachable"};

                        case Opcode::jump:
                            bytecode_iter += jump_distance;
                            break;

                        case Opcode::jump_if_false:
                            if (! std::visit(Truthy_visitor{}, stack_.back().variant)) {
                                bytecode_iter += jump_distance;
                            }
                            break;

                        case Opcode::loop:
                            bytecode_iter -= jump_distance;
                            break;
                    }

                    break;
                }

                case Opcode::less: {
                    const auto& b = *(stack_.cend() - 1);
                    const auto& a = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(a.variant) || ! std::holds_alternative<double>(b.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(a.variant) < std::get<double>(b.variant);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(Dynamic_type_value{result});

                    ++bytecode_iter;

                    break;
                }

                case Opcode::multiply: {
                    const auto& b = *(stack_.cend() - 1);
                    const auto& a = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(a.variant) || ! std::holds_alternative<double>(b.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(a.variant) * std::get<double>(b.variant);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(Dynamic_type_value{result});

                    ++bytecode_iter;

                    break;
                }

                case Opcode::negate: {
                    if (! std::holds_alternative<double>(stack_.back().variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operand must be a number."
                        };
                    }

                    const auto negated_value = - std::get<double>(stack_.back().variant);

                    stack_.pop_back();
                    stack_.push_back(Dynamic_type_value{negated_value});

                    ++bytecode_iter;

                    break;
                }

                case Opcode::nil: {
                    stack_.push_back(Dynamic_type_value{nullptr});

                    ++bytecode_iter;

                    break;
                }

                case Opcode::not_: {
                    const auto negated_value = ! std::visit(Truthy_visitor{}, stack_.back().variant);

                    stack_.pop_back();
                    stack_.push_back(Dynamic_type_value{negated_value});

                    ++bytecode_iter;

                    break;
                }

                case Opcode::pop: {
                    stack_.pop_back();

                    ++bytecode_iter;

                    break;
                }

                case Opcode::print: {
                    os << stack_.back() << "\n";
                    stack_.pop_back();

                    ++bytecode_iter;

                    break;
                }

                case Opcode::set_global: {
                    const auto variable_name_constant_index = *(bytecode_iter + 1);
                    const auto& variable_name = std::get<std::string>(chunk.constants().at(variable_name_constant_index).variant);

                    const auto global_iter = globals_.find(variable_name);
                    if (global_iter == globals_.end()) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Undefined variable '" + variable_name + "'."
                        };
                    }
                    global_iter->second = stack_.back();

                    bytecode_iter += 2;

                    break;
                }

                case Opcode::subtract: {
                    const auto& b = *(stack_.cend() - 1);
                    const auto& a = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(a.variant) || ! std::holds_alternative<double>(b.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(a.variant) - std::get<double>(b.variant);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(Dynamic_type_value{result});

                    ++bytecode_iter;

                    break;
                }

                case Opcode::true_: {
                    stack_.push_back(Dynamic_type_value{true});

                    ++bytecode_iter;

                    break;
                }
            }

            if (debug) {
                os << "Stack:\n";
                for (auto stack_iter = stack_.crbegin(); stack_iter != stack_.crend(); ++stack_iter) {
                    const auto stack_index = stack_iter.base() - 1 - stack_.cbegin();
                    os << std::setw(5) << std::setfill(' ') << std::right << stack_index << " : " << *stack_iter << "\n";
                }
                os << "\n";
            }
        }
    }
}}
