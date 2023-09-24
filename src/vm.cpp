#include "vm.hpp"

#include <iomanip>
#include <boost/endian/conversion.hpp>
#include "object.hpp"

namespace {
    using namespace motts::lox;

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

    struct Mark_objects_visitor {
        GC_heap& gc_heap;

        Mark_objects_visitor(GC_heap& gc_heap_arg)
            : gc_heap {gc_heap_arg}
        {}

        auto operator()(GC_ptr<Function> fn) {
            mark(gc_heap, fn);
        }

        template<typename T> auto operator()(const T&) {}
    };
}

namespace motts { namespace lox {
    VM::VM(GC_heap& gc_heap, std::ostream& os, bool debug)
        : debug_ {debug},
          os_ {os},
          gc_heap_ {gc_heap}
    {
        gc_heap_.on_mark_roots.push_back([&] {
            for (const auto& root_value : stack_) {
                std::visit(Mark_objects_visitor{gc_heap_}, root_value.variant);
            }

            for (const auto& [_, root_value] : globals_) {
                std::visit(Mark_objects_visitor{gc_heap_}, root_value.variant);
            }
        });
    }

    VM::~VM() {
        gc_heap_.on_mark_roots.pop_back();
    }

    void VM::run(const Chunk& chunk) {
        call_frames_.push_back({chunk, chunk.bytecode().cbegin(), 0});

        if (debug_) {
            os_ << "\n# Running chunk:\n\n" << call_frames_.back().chunk << '\n';
        }

        while (call_frames_.back().bytecode_iter != call_frames_.back().chunk.bytecode().cend()) {
            const auto bytecode_index = call_frames_.back().bytecode_iter - call_frames_.back().chunk.bytecode().cbegin();
            const auto& source_map_token = call_frames_.back().chunk.source_map_tokens().at(bytecode_index);

            const auto& opcode = static_cast<Opcode>(*call_frames_.back().bytecode_iter);
            switch (opcode) {
                default: {
                    std::ostringstream os;
                    os << "[Line " << source_map_token.line << "] Error: Unexpected opcode " << opcode
                        << ", generated from source \"" << source_map_token.lexeme << "\".";
                    throw std::runtime_error{os.str()};
                }

                case Opcode::add: {
                    const auto& rhs = *(stack_.cend() - 1);
                    const auto& lhs = *(stack_.cend() - 2);

                    if (std::holds_alternative<double>(lhs.variant) && std::holds_alternative<double>(rhs.variant))
                    {
                        const auto result = std::get<double>(lhs.variant) + std::get<double>(rhs.variant);
                        stack_.erase(stack_.cend() - 2, stack_.cend());
                        stack_.push_back(Dynamic_type_value{result});
                    }
                    else if (std::holds_alternative<std::string>(lhs.variant) && std::holds_alternative<std::string>(rhs.variant))
                    {
                        auto result = std::get<std::string>(lhs.variant) + std::get<std::string>(rhs.variant);
                        stack_.erase(stack_.cend() - 2, stack_.cend());
                        stack_.push_back(Dynamic_type_value{std::move(result)});
                    }
                    else
                    {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be two numbers or two strings."
                        };
                    }

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::call: {
                    const auto arg_count = *(call_frames_.back().bytecode_iter + 1);
                    call_frames_.back().bytecode_iter += 2;

                    const auto& maybe_callable = *(stack_.cend() - arg_count - 1);
                    if (! std::holds_alternative<GC_ptr<Function>>(maybe_callable.variant)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": "
                            << "Can only call functions.";
                        throw std::runtime_error{os.str()};
                    }

                    const auto& callable = std::get<GC_ptr<Function>>(maybe_callable.variant);
                    if (callable->arity != arg_count) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": "
                            << "Expected " << callable->arity << " arguments but got " << static_cast<int>(arg_count) << '.';
                        throw std::runtime_error{os.str()};
                    }

                    call_frames_.push_back({callable->chunk, callable->chunk.bytecode().cbegin(), stack_.size() - arg_count - 1});

                    break;
                }

                case Opcode::constant: {
                    const auto constant_index = *(call_frames_.back().bytecode_iter + 1);
                    stack_.push_back(call_frames_.back().chunk.constants().at(constant_index));
                    call_frames_.back().bytecode_iter += 2;
                    break;
                }

                case Opcode::divide: {
                    const auto& rhs = *(stack_.cend() - 1);
                    const auto& lhs = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(lhs.variant) || ! std::holds_alternative<double>(rhs.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(lhs.variant) / std::get<double>(rhs.variant);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(Dynamic_type_value{result});

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::equal: {
                    const auto& rhs = *(stack_.cend() - 1);
                    const auto& lhs = *(stack_.cend() - 2);

                    const auto result = lhs.variant == rhs.variant;
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(Dynamic_type_value{result});

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::false_: {
                    stack_.push_back(Dynamic_type_value{false});
                    ++call_frames_.back().bytecode_iter;
                    break;
                }

                case Opcode::define_global: {
                    const auto variable_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& variable_name = std::get<std::string>(call_frames_.back().chunk.constants().at(variable_name_constant_index).variant);

                    globals_[variable_name] = stack_.back();
                    stack_.pop_back();

                    call_frames_.back().bytecode_iter += 2;

                    break;
                }

                case Opcode::get_global: {
                    const auto variable_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& variable_name = std::get<std::string>(call_frames_.back().chunk.constants().at(variable_name_constant_index).variant);

                    const auto global_iter = globals_.find(variable_name);
                    if (global_iter == globals_.end()) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Undefined variable '" + variable_name + "'."
                        };
                    }
                    stack_.push_back(global_iter->second);

                    call_frames_.back().bytecode_iter += 2;

                    break;
                }

                case Opcode::get_local: {
                    const auto local_stack_index = *(call_frames_.back().bytecode_iter + 1);
                    stack_.push_back(stack_.at(call_frames_.back().stack_begin_index + local_stack_index));
                    call_frames_.back().bytecode_iter += 2;
                    break;
                }

                case Opcode::greater: {
                    const auto& rhs = *(stack_.cend() - 1);
                    const auto& lhs = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(lhs.variant) || ! std::holds_alternative<double>(rhs.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(lhs.variant) > std::get<double>(rhs.variant);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(Dynamic_type_value{result});

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::jump:
                case Opcode::jump_if_false:
                case Opcode::loop: {
                    const auto jump_distance_big_endian = reinterpret_cast<const std::uint16_t&>(*(call_frames_.back().bytecode_iter + 1));
                    const auto jump_distance = boost::endian::big_to_native(jump_distance_big_endian);
                    call_frames_.back().bytecode_iter += 3;

                    switch (opcode) {
                        default:
                            throw std::logic_error{"Unreachable."};

                        case Opcode::jump:
                            call_frames_.back().bytecode_iter += jump_distance;
                            break;

                        case Opcode::jump_if_false:
                            if (! std::visit(Truthy_visitor{}, stack_.back().variant)) {
                                call_frames_.back().bytecode_iter += jump_distance;
                            }
                            break;

                        case Opcode::loop:
                            call_frames_.back().bytecode_iter -= jump_distance;
                            break;
                    }

                    break;
                }

                case Opcode::less: {
                    const auto& rhs = *(stack_.cend() - 1);
                    const auto& lhs = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(lhs.variant) || ! std::holds_alternative<double>(rhs.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(lhs.variant) < std::get<double>(rhs.variant);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(Dynamic_type_value{result});

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::multiply: {
                    const auto& rhs = *(stack_.cend() - 1);
                    const auto& lhs = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(lhs.variant) || ! std::holds_alternative<double>(rhs.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(lhs.variant) * std::get<double>(rhs.variant);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(Dynamic_type_value{result});

                    ++call_frames_.back().bytecode_iter;

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

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::nil: {
                    stack_.push_back(Dynamic_type_value{nullptr});
                    ++call_frames_.back().bytecode_iter;
                    break;
                }

                case Opcode::not_: {
                    const auto negated_value = ! std::visit(Truthy_visitor{}, stack_.back().variant);

                    stack_.pop_back();
                    stack_.push_back(Dynamic_type_value{negated_value});

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::pop: {
                    stack_.pop_back();
                    ++call_frames_.back().bytecode_iter;
                    break;
                }

                case Opcode::print: {
                    os_ << stack_.back() << '\n';
                    stack_.pop_back();

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::return_: {
                    stack_.erase(stack_.cbegin() + call_frames_.back().stack_begin_index, stack_.cend() - 1);
                    call_frames_.pop_back();

                    break;
                }

                case Opcode::set_global: {
                    const auto variable_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& variable_name = std::get<std::string>(call_frames_.back().chunk.constants().at(variable_name_constant_index).variant);

                    const auto global_iter = globals_.find(variable_name);
                    if (global_iter == globals_.end()) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Undefined variable '" + variable_name + "'."
                        };
                    }
                    global_iter->second = stack_.back();

                    call_frames_.back().bytecode_iter += 2;

                    break;
                }

                case Opcode::set_local: {
                    const auto local_stack_index = *(call_frames_.back().bytecode_iter + 1);
                    stack_.at(call_frames_.back().stack_begin_index + local_stack_index) = stack_.back();

                    call_frames_.back().bytecode_iter += 2;

                    break;
                }

                case Opcode::subtract: {
                    const auto& rhs = *(stack_.cend() - 1);
                    const auto& lhs = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(lhs.variant) || ! std::holds_alternative<double>(rhs.variant)) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Operands must be numbers."
                        };
                    }

                    const auto result = std::get<double>(lhs.variant) - std::get<double>(rhs.variant);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(Dynamic_type_value{result});

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::true_: {
                    stack_.push_back(Dynamic_type_value{true});
                    ++call_frames_.back().bytecode_iter;
                    break;
                }
            }

            if (debug_) {
                os_ << "Stack:\n";
                for (auto stack_iter = stack_.crbegin(); stack_iter != stack_.crend(); ++stack_iter) {
                    const auto stack_index = stack_iter.base() - 1 - stack_.cbegin();
                    os_ << std::setw(5) << std::setfill(' ') << std::right << stack_index << " : " << *stack_iter << '\n';
                }
                os_ << '\n';
            }
        }

        gc_heap_.collect_garbage();
    }
}}
