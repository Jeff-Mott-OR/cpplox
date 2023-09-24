#include "vm.hpp"

#include <iomanip>

#include <boost/endian/conversion.hpp>
#include <gsl/gsl_util>

#include "object.hpp"

namespace motts { namespace lox {
    VM::VM(GC_heap& gc_heap, std::ostream& os, bool debug)
        : debug_ {debug},
          os_ {os},
          gc_heap_ {gc_heap}
    {
        gc_heap_.on_mark_roots.push_back([&] {
            for (const auto& call_frame : call_frames_) {
                mark(gc_heap_, call_frame.closure);
                for (const auto& root_value : call_frame.chunk.constants()) {
                    std::visit(Dynamic_type_value::Mark_objects_visitor{gc_heap_}, root_value.variant);
                }
            }

            for (const auto& root_value : stack_) {
                std::visit(Dynamic_type_value::Mark_objects_visitor{gc_heap_}, root_value.variant);
            }

            for (const auto& [_, root_value] : globals_) {
                std::visit(Dynamic_type_value::Mark_objects_visitor{gc_heap_}, root_value.variant);
            }
        });
    }

    VM::~VM() {
        gc_heap_.on_mark_roots.pop_back();
    }

    void VM::run(const Chunk& chunk) {
        call_frames_.push_back({{}, chunk, chunk.bytecode().cbegin(), 0});
        const auto _ = gsl::finally([&] {
            call_frames_.pop_back();
        });

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
                    const auto& arg_count = *(call_frames_.back().bytecode_iter + 1);
                    call_frames_.back().bytecode_iter += 2;

                    auto& maybe_callable = *(stack_.end() - arg_count - 1);

                    if (std::holds_alternative<GC_ptr<Closure>>(maybe_callable.variant))
                    {
                        const auto& closure = std::get<GC_ptr<Closure>>(maybe_callable.variant);
                        if (closure->function->arity != arg_count) {
                            std::ostringstream os;
                            os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": "
                                << "Expected " << closure->function->arity << " arguments but got " << static_cast<int>(arg_count) << '.';
                            throw std::runtime_error{os.str()};
                        }

                        call_frames_.push_back({
                            closure,
                            closure->function->chunk,
                            closure->function->chunk.bytecode().cbegin(),
                            stack_.size() - arg_count - 1
                        });
                    }
                    else if (std::holds_alternative<GC_ptr<Class>>(maybe_callable.variant))
                    {
                        const auto class_ = std::get<GC_ptr<Class>>(maybe_callable.variant);
                        const auto maybe_init_iter = class_->methods.find("init");
                        const auto expected_arity = maybe_init_iter != class_->methods.cend() ? maybe_init_iter->second->function->arity : 0;

                        if (arg_count != expected_arity) {
                            std::ostringstream os;
                            os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": "
                                << "Expected " << expected_arity << " arguments but got " << static_cast<int>(arg_count) << '.';
                            throw std::runtime_error{os.str()};
                        }

                        // If there's no init method, then we'd pop the class and push the instance,
                        // so assigning over the class slot has the same effect.
                        // But if there's an init method, then we need to prepare the stack like a bound method,
                        // which means putting the "this" instance in the class slot before the arguments.
                        // Either way, the instance ends up in the same slot where the class was.
                        maybe_callable.variant = gc_heap_.make<Instance>({class_});

                        if (maybe_init_iter != class_->methods.cend()) {
                            call_frames_.push_back({
                                maybe_init_iter->second,
                                maybe_init_iter->second->function->chunk,
                                maybe_init_iter->second->function->chunk.bytecode().cbegin(),
                                stack_.size() - arg_count - 1
                            });
                        }
                    }
                    else if (std::holds_alternative<GC_ptr<Bound_method>>(maybe_callable.variant))
                    {
                        const auto& bound_method = std::get<GC_ptr<Bound_method>>(maybe_callable.variant);

                        if (bound_method->closure->function->arity != arg_count) {
                            std::ostringstream os;
                            os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": "
                                << "Expected " << bound_method->closure->function->arity << " arguments but got " << static_cast<int>(arg_count) << '.';
                            throw std::runtime_error{os.str()};
                        }

                        call_frames_.push_back({
                            bound_method->closure,
                            bound_method->closure->function->chunk,
                            bound_method->closure->function->chunk.bytecode().cbegin(),
                            stack_.size() - arg_count - 1
                        });

                        // Replace function at call frame stack slot 0 with "this" instance at slot 0.
                        maybe_callable.variant = bound_method->instance;
                    }
                    else
                    {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": "
                            << "Can only call functions.";
                        throw std::runtime_error{os.str()};
                    }

                    break;
                }

                case Opcode::class_: {
                    const auto& class_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& class_name = std::get<std::string>(call_frames_.back().chunk.constants().at(class_name_constant_index).variant);
                    stack_.push_back(Dynamic_type_value{gc_heap_.make<Class>({class_name})});
                    call_frames_.back().bytecode_iter += 2;
                    break;
                }

                case Opcode::close_upvalue: {
                    call_frames_.back().closure->open_upvalues.back()->close();
                    call_frames_.back().closure->open_upvalues.pop_back();
                    stack_.pop_back();
                    ++call_frames_.back().bytecode_iter;
                    break;
                }

                case Opcode::closure: {
                    const auto& fn_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& n_upvalues = *(call_frames_.back().bytecode_iter + 2);

                    const auto& function = std::get<GC_ptr<Function>>(call_frames_.back().chunk.constants().at(fn_constant_index).variant);
                    auto new_closure = gc_heap_.make<Closure>({function});

                    for (auto n_upvalue = 0; n_upvalue != n_upvalues; ++n_upvalue) {
                        const auto& is_direct_capture = *(call_frames_.back().bytecode_iter + 3 + 2 * n_upvalue);
                        const auto& enclosing_index = *(call_frames_.back().bytecode_iter + 3 + 2 * n_upvalue + 1);

                        if (is_direct_capture) {
                            const auto stack_index = call_frames_.back().stack_begin_index + enclosing_index;

                            const auto maybe_existing_upvalue_iter = std::find_if(
                                call_frames_.back().closure->open_upvalues.cbegin(), call_frames_.back().closure->open_upvalues.cend(),
                                [&] (const auto& open_upvalue) {
                                    return open_upvalue->stack_index == stack_index;
                                }
                            );

                            if (maybe_existing_upvalue_iter != call_frames_.back().closure->open_upvalues.cend()) {
                                new_closure->upvalues.push_back(*maybe_existing_upvalue_iter);
                            } else {
                                const auto new_upvalue = gc_heap_.make<Upvalue>({stack_, stack_index});

                                // The new closure keeps upvalue for lookups.
                                new_closure->upvalues.push_back(new_upvalue);

                                // The enclosing closure keeps upvalue for auto-closing on scope exit.
                                const auto sorted_insert_position = std::find_if(
                                    call_frames_.back().closure->open_upvalues.cbegin(), call_frames_.back().closure->open_upvalues.cend(),
                                    [&] (const auto& open_upvalue) {
                                        return open_upvalue->stack_index > stack_index;
                                    }
                                );
                                call_frames_.back().closure->open_upvalues.insert(sorted_insert_position, new_upvalue);
                            }
                        } else {
                            new_closure->upvalues.push_back(call_frames_.back().closure->upvalues.at(enclosing_index));
                        }
                    }

                    stack_.push_back(Dynamic_type_value{new_closure});
                    call_frames_.back().bytecode_iter += 3 + 2 * n_upvalues;

                    break;
                }

                case Opcode::constant: {
                    const auto& constant_index = *(call_frames_.back().bytecode_iter + 1);
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
                    const auto& variable_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& variable_name = std::get<std::string>(call_frames_.back().chunk.constants().at(variable_name_constant_index).variant);
                    globals_[variable_name] = stack_.back();
                    stack_.pop_back();
                    call_frames_.back().bytecode_iter += 2;
                    break;
                }

                case Opcode::get_global: {
                    const auto& variable_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& variable_name = std::get<std::string>(call_frames_.back().chunk.constants().at(variable_name_constant_index).variant);

                    const auto global_iter = globals_.find(variable_name);
                    if (global_iter == globals_.cend()) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Undefined variable '" + variable_name + "'."
                        };
                    }
                    stack_.push_back(global_iter->second);

                    call_frames_.back().bytecode_iter += 2;

                    break;
                }

                case Opcode::get_local: {
                    const auto& local_stack_index = *(call_frames_.back().bytecode_iter + 1);
                    stack_.push_back(stack_.at(call_frames_.back().stack_begin_index + local_stack_index));
                    call_frames_.back().bytecode_iter += 2;
                    break;
                }

                case Opcode::get_property: {
                    const auto& field_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& field_name = std::get<std::string>(call_frames_.back().chunk.constants().at(field_name_constant_index).variant);
                    const auto& instance = std::get<GC_ptr<Instance>>(stack_.back().variant);
                    call_frames_.back().bytecode_iter += 2;

                    const auto maybe_field_iter = instance->fields.find(field_name);
                    if (maybe_field_iter != instance->fields.cend()) {
                        stack_.pop_back();
                        stack_.push_back(maybe_field_iter->second);

                        break;
                    }

                    const auto maybe_method_iter = instance->class_->methods.find(field_name);
                    if (maybe_method_iter != instance->class_->methods.cend()) {
                        const auto new_bound_method = gc_heap_.make<Bound_method>({instance, maybe_method_iter->second});
                        stack_.pop_back();
                        stack_.push_back(Dynamic_type_value{new_bound_method});

                        break;
                    }

                    throw std::runtime_error{
                        "[Line " + std::to_string(source_map_token.line) + "] Error: Undefined property '" + field_name + "'."
                    };
                }

                case Opcode::get_upvalue: {
                    const auto& upvalue_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& upvalue = call_frames_.back().closure->upvalues.at(upvalue_index);
                    stack_.push_back(upvalue->value());
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
                    const auto& jump_distance_big_endian = reinterpret_cast<const std::uint16_t&>(*(call_frames_.back().bytecode_iter + 1));
                    const auto jump_distance = boost::endian::big_to_native(jump_distance_big_endian);
                    call_frames_.back().bytecode_iter += 3;

                    switch (opcode) {
                        default:
                            throw std::logic_error{"Unreachable."};

                        case Opcode::jump:
                            call_frames_.back().bytecode_iter += jump_distance;
                            break;

                        case Opcode::jump_if_false:
                            if (! std::visit(Dynamic_type_value::Truthy_visitor{}, stack_.back().variant)) {
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

                case Opcode::method: {
                    const auto& method_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& method_name = std::get<std::string>(call_frames_.back().chunk.constants().at(method_name_constant_index).variant);
                    auto& class_ = std::get<GC_ptr<Class>>((stack_.end() - 2)->variant);

                    class_->methods[method_name] = std::get<GC_ptr<Closure>>(stack_.back().variant);
                    stack_.pop_back();

                    call_frames_.back().bytecode_iter += 2;

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
                    const auto negated_value = ! std::visit(Dynamic_type_value::Truthy_visitor{}, stack_.back().variant);
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
                    for (auto& upvalue : call_frames_.back().closure->open_upvalues) {
                        upvalue->close();
                    }
                    stack_.erase(stack_.cbegin() + call_frames_.back().stack_begin_index, stack_.cend() - 1);
                    call_frames_.pop_back();
                    break;
                }

                case Opcode::set_global: {
                    const auto& variable_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
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
                    const auto& local_stack_index = *(call_frames_.back().bytecode_iter + 1);
                    stack_.at(call_frames_.back().stack_begin_index + local_stack_index) = stack_.back();
                    call_frames_.back().bytecode_iter += 2;
                    break;
                }

                case Opcode::set_property: {
                    const auto& field_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& field_name = std::get<std::string>(call_frames_.back().chunk.constants().at(field_name_constant_index).variant);
                    auto& instance = std::get<GC_ptr<Instance>>(stack_.back().variant);
                    const auto& new_value = *(stack_.cend() - 2);

                    instance->fields[field_name] = new_value;
                    stack_.pop_back();

                    call_frames_.back().bytecode_iter += 2;

                    break;
                }

                case Opcode::set_upvalue: {
                    const auto& upvalue_index = *(call_frames_.back().bytecode_iter + 1);
                    auto& upvalue = call_frames_.back().closure->upvalues.at(upvalue_index);
                    upvalue->value() = stack_.back();
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

            gc_heap_.collect_garbage();

            if (debug_) {
                os_ << "Stack:\n";
                for (auto stack_iter = stack_.crbegin(); stack_iter != stack_.crend(); ++stack_iter) {
                    const auto stack_index = stack_iter.base() - 1 - stack_.cbegin();
                    os_ << std::setw(5) << std::setfill(' ') << std::right << stack_index << " : " << *stack_iter << '\n';
                }
                os_ << '\n';
            }
        }
    }
}}
