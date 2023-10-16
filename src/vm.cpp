#include "vm.hpp"

#include <chrono>
#include <iomanip>

#include <boost/endian/conversion.hpp>
#include <gsl/gsl>

#include "object.hpp"

namespace motts::lox
{
    static Dynamic_type_value clock_native(std::span<Dynamic_type_value>)
    {
        const auto now_time_point = std::chrono::system_clock::now().time_since_epoch();
        const auto now_seconds = std::chrono::duration_cast<std::chrono::seconds>(now_time_point).count();
        return gsl::narrow<double>(now_seconds);
    }

    VM::VM(GC_heap& gc_heap, Interned_strings& interned_strings, std::ostream& os, bool debug)
        : debug_{debug},
          os_{os},
          gc_heap_{gc_heap},
          interned_strings_{interned_strings}
    {
        gc_heap_.on_mark_roots.push_back([this] {
            for (const GC_ptr<Closure> closure : call_frames_) {
                mark(gc_heap_, closure);
            }

            for (const Dynamic_type_value value : stack_) {
                std::visit(Mark_objects_visitor{gc_heap_}, value);
            }

            for (const auto& [key, value] : globals_) {
                mark(gc_heap_, key);
                std::visit(Mark_objects_visitor{gc_heap_}, value);
            }
        });

        globals_[interned_strings_.get("clock")] = gc_heap_.make<Native_fn>({clock_native});
    }

    VM::~VM()
    {
        gc_heap_.on_mark_roots.pop_back();
    }

    void VM::run(const Chunk& chunk)
    {
        if (debug_) {
            os_ << "\n# Running chunk:\n\n" << chunk << '\n';
        }

        const auto root_script_fn = gc_heap_.make<Closure>(gc_heap_.make<Function>({interned_strings_.get(""), 0, chunk}));
        run(root_script_fn, 0);
    }

    void VM::run(GC_ptr<Closure> closure, std::size_t stack_begin_index)
    {
        call_frames_.push_back(closure);
        const auto _ = gsl::finally([&] { call_frames_.pop_back(); });

        const auto& chunk = closure->function->chunk;
        const auto& bytecode = chunk.bytecode();
        const auto& constants = chunk.constants();
        const auto& source_map_tokens = chunk.source_map_tokens();

        auto& upvalues = closure->upvalues;
        auto& open_upvalues = closure->open_upvalues;

        const auto bytecode_begin = bytecode.cbegin();
        const auto bytecode_end = bytecode.cend();

        auto bytecode_iter = bytecode_begin;

        while (bytecode_iter != bytecode_end) {
            const auto bytecode_index = bytecode_iter - bytecode_begin;
            const auto& source_map_token = source_map_tokens[bytecode_index];

            const auto opcode = static_cast<Opcode>(*bytecode_iter++);
            switch (opcode) {
                default: {
                    std::ostringstream os;
                    os << "[Line " << source_map_token.line << "] Error: Unexpected opcode " << opcode << ", generated from source \""
                       << *source_map_token.lexeme << "\".";
                    throw std::runtime_error{os.str()};
                }

                case Opcode::add: {
                    const Dynamic_type_value rhs = *(stack_.cend() - 1);
                    const Dynamic_type_value lhs = *(stack_.cend() - 2);

                    if (std::holds_alternative<double>(lhs) && std::holds_alternative<double>(rhs)) {
                        const auto result = std::get<double>(lhs) + std::get<double>(rhs);
                        stack_.erase(stack_.cend() - 2, stack_.cend());
                        stack_.push_back(result);
                    } else if (std::holds_alternative<GC_ptr<const std::string>>(lhs) && std::holds_alternative<GC_ptr<const std::string>>(rhs))
                    {
                        auto result = *std::get<GC_ptr<const std::string>>(lhs) + *std::get<GC_ptr<const std::string>>(rhs);
                        stack_.erase(stack_.cend() - 2, stack_.cend());
                        stack_.push_back(interned_strings_.get(std::move(result)));
                    } else {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << *source_map_token.lexeme
                           << "\": Operands must be two numbers or two strings.";
                        throw std::runtime_error{os.str()};
                    }

                    break;
                }

                case Opcode::call: {
                    const auto arg_count = *bytecode_iter++;
                    const Dynamic_type_value maybe_callable = *(stack_.end() - arg_count - 1);

                    if (std::holds_alternative<GC_ptr<Closure>>(maybe_callable)) {
                        const auto closure = std::get<GC_ptr<Closure>>(maybe_callable);

                        if (closure->function->arity != arg_count) {
                            std::ostringstream os;
                            os << "[Line " << source_map_token.line << "] Error at \"" << *source_map_token.lexeme << "\": "
                               << "Expected " << closure->function->arity << " arguments but got " << static_cast<int>(arg_count) << '.';
                            throw std::runtime_error{os.str()};
                        }

                        run(closure, stack_.size() - arg_count - 1);
                    } else if (std::holds_alternative<GC_ptr<Class>>(maybe_callable)) {
                        const auto klass = std::get<GC_ptr<Class>>(maybe_callable);
                        const auto maybe_init_iter = klass->methods.find(interned_strings_.get("init"));
                        const auto arity = maybe_init_iter != klass->methods.cend() ? maybe_init_iter->second->function->arity : 0;

                        if (arity != arg_count) {
                            std::ostringstream os;
                            os << "[Line " << source_map_token.line << "] Error at \"" << *source_map_token.lexeme << "\": "
                               << "Expected " << arity << " arguments but got " << static_cast<int>(arg_count) << '.';
                            throw std::runtime_error{os.str()};
                        }

                        // If there's no init method, then we'd pop the class and push the instance,
                        // so assigning the instance into the class slot has the same effect.
                        // But if there's an init method, then we need to prepare the stack like a bound method,
                        // which means putting the "this" instance in the class slot before the arguments.
                        // Either way, the instance ends up in the same slot where the class was.
                        *(stack_.end() - arg_count - 1) = gc_heap_.make<Instance>({klass});

                        if (maybe_init_iter != klass->methods.cend()) {
                            run(maybe_init_iter->second, stack_.size() - arg_count - 1);
                        }
                    } else if (std::holds_alternative<GC_ptr<Bound_method>>(maybe_callable)) {
                        const auto bound_method = std::get<GC_ptr<Bound_method>>(maybe_callable);

                        if (bound_method->method->function->arity != arg_count) {
                            std::ostringstream os;
                            os << "[Line " << source_map_token.line << "] Error at \"" << *source_map_token.lexeme << "\": "
                               << "Expected " << bound_method->method->function->arity << " arguments but got "
                               << static_cast<int>(arg_count) << '.';
                            throw std::runtime_error{os.str()};
                        }

                        // Replace function at call frame stack slot 0 with "this" instance.
                        *(stack_.end() - arg_count - 1) = bound_method->instance;

                        run(bound_method->method, stack_.size() - arg_count - 1);
                    } else if (std::holds_alternative<GC_ptr<Native_fn>>(maybe_callable)) {
                        const auto native_fn = std::get<GC_ptr<Native_fn>>(maybe_callable);
                        const auto return_value = native_fn->fn({stack_.end() - arg_count, stack_.end()});
                        stack_.erase(stack_.end() - arg_count - 1, stack_.end());
                        stack_.push_back(return_value);
                    } else {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << *source_map_token.lexeme << "\": "
                           << "Can only call functions and classes.";
                        throw std::runtime_error{os.str()};
                    }

                    break;
                }

                case Opcode::class_: {
                    const auto class_name_constant_index = *bytecode_iter++;
                    const auto class_name = std::get<GC_ptr<const std::string>>(constants[class_name_constant_index]);
                    stack_.push_back(gc_heap_.make<Class>({class_name}));

                    break;
                }

                case Opcode::close_upvalue: {
                    // At compile-time, we lexically know we *might* capture an upvalue, and thus emit a close instruction instead of pop.
                    // But at runtime, the closure function might be conditional and might never create an open upvalue,
                    // so we need to check that we're not trying to close an upvalue that was never opened.
                    if (! open_upvalues.empty() && open_upvalues.back()->stack_index() == stack_.size() - 1) {
                        open_upvalues.back()->close();
                        open_upvalues.pop_back();
                    }
                    stack_.pop_back();

                    break;
                }

                case Opcode::closure: {
                    const auto fn_constant_index = *bytecode_iter++;
                    const auto function = std::get<GC_ptr<Function>>(constants[fn_constant_index]);
                    auto new_closure = gc_heap_.make<Closure>({function});
                    stack_.push_back(new_closure);

                    const auto n_upvalues = *bytecode_iter++;
                    for (auto n_upvalue = 0; n_upvalue != n_upvalues; ++n_upvalue) {
                        const auto is_direct_capture = *bytecode_iter++;
                        const auto enclosing_index = *bytecode_iter++;

                        if (is_direct_capture) {
                            const auto stack_index = stack_begin_index + enclosing_index;

                            const auto maybe_existing_upvalue_iter =
                                std::find_if(open_upvalues.cbegin(), open_upvalues.cend(), [&](const GC_ptr<Upvalue> open_upvalue) {
                                    return open_upvalue->stack_index() == stack_index;
                                });

                            if (maybe_existing_upvalue_iter != open_upvalues.cend()) {
                                new_closure->upvalues.push_back(*maybe_existing_upvalue_iter);
                            } else {
                                const auto new_upvalue = gc_heap_.make<Upvalue>({stack_, stack_index});

                                // The new closure keeps upvalue for lookups.
                                new_closure->upvalues.push_back(new_upvalue);

                                // The enclosing closure keeps upvalue for auto-closing on scope exit.
                                const auto sorted_insert_position =
                                    std::find_if(open_upvalues.cbegin(), open_upvalues.cend(), [&](const GC_ptr<Upvalue> open_upvalue) {
                                        return open_upvalue->stack_index() > stack_index;
                                    });
                                open_upvalues.insert(sorted_insert_position, new_upvalue);
                            }
                        } else {
                            new_closure->upvalues.push_back(upvalues[enclosing_index]);
                        }
                    }

                    break;
                }

                case Opcode::constant: {
                    const auto constant_index = *bytecode_iter++;
                    stack_.push_back(constants[constant_index]);

                    break;
                }

                case Opcode::divide: {
                    const Dynamic_type_value rhs = *(stack_.cend() - 1);
                    const Dynamic_type_value lhs = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(lhs) || ! std::holds_alternative<double>(rhs)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << *source_map_token.lexeme
                           << "\": Operands must be numbers.";
                        throw std::runtime_error{os.str()};
                    }

                    const auto result = std::get<double>(lhs) / std::get<double>(rhs);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(result);

                    break;
                }

                case Opcode::equal: {
                    const Dynamic_type_value rhs = *(stack_.cend() - 1);
                    const Dynamic_type_value lhs = *(stack_.cend() - 2);

                    const auto result = lhs == rhs;
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(result);

                    break;
                }

                case Opcode::false_: {
                    stack_.push_back(false);
                    break;
                }

                case Opcode::define_global: {
                    const auto variable_name_constant_index = *bytecode_iter++;
                    const auto variable_name = std::get<GC_ptr<const std::string>>(constants[variable_name_constant_index]);
                    globals_[variable_name] = stack_.back();
                    stack_.pop_back();

                    break;
                }

                case Opcode::get_global: {
                    const auto variable_name_constant_index = *bytecode_iter++;
                    const auto variable_name = std::get<GC_ptr<const std::string>>(constants[variable_name_constant_index]);

                    const auto global_iter = globals_.find(variable_name);
                    if (global_iter == globals_.cend()) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Undefined variable \"" + *variable_name + "\"."};
                    }
                    stack_.push_back(global_iter->second);

                    break;
                }

                case Opcode::get_local: {
                    const auto local_stack_index = *bytecode_iter++;
                    stack_.push_back(stack_[stack_begin_index + local_stack_index]);

                    break;
                }

                case Opcode::get_property: {
                    const auto field_name_constant_index = *bytecode_iter++;
                    const auto field_name = std::get<GC_ptr<const std::string>>(constants[field_name_constant_index]);

                    const Dynamic_type_value maybe_instance = stack_.back();
                    if (! std::holds_alternative<GC_ptr<Instance>>(maybe_instance)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << *source_map_token.lexeme
                           << "\": Only instances have fields.";
                        throw std::runtime_error{os.str()};
                    }
                    const auto instance = std::get<GC_ptr<Instance>>(maybe_instance);

                    const auto maybe_field_iter = instance->fields.find(field_name);
                    if (maybe_field_iter != instance->fields.cend()) {
                        stack_.pop_back();
                        stack_.push_back(maybe_field_iter->second);

                        break;
                    }

                    const auto maybe_method_iter = instance->klass->methods.find(field_name);
                    if (maybe_method_iter != instance->klass->methods.cend()) {
                        const auto new_bound_method = gc_heap_.make<Bound_method>({instance, maybe_method_iter->second});
                        stack_.pop_back();
                        stack_.push_back(new_bound_method);

                        break;
                    }

                    throw std::runtime_error{
                        "[Line " + std::to_string(source_map_token.line) + "] Error: Undefined property \"" + *field_name + "\"."};
                }

                case Opcode::get_super: {
                    const auto method_name_constant_index = *bytecode_iter++;
                    const auto method_name = std::get<GC_ptr<const std::string>>(constants[method_name_constant_index]);
                    const auto superclass = std::get<GC_ptr<Class>>(*(stack_.cend() - 1));
                    const auto instance = std::get<GC_ptr<Instance>>(*(stack_.cend() - 2));

                    const auto maybe_method_iter = superclass->methods.find(method_name);
                    if (maybe_method_iter == superclass->methods.cend()) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Undefined property \"" + *method_name + "\"."};
                    }

                    const auto new_bound_method = gc_heap_.make<Bound_method>({instance, maybe_method_iter->second});
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(new_bound_method);

                    break;
                }

                case Opcode::get_upvalue: {
                    const auto upvalue_index = *bytecode_iter++;
                    stack_.push_back(upvalues[upvalue_index]->value());

                    break;
                }

                case Opcode::greater: {
                    const Dynamic_type_value rhs = *(stack_.cend() - 1);
                    const Dynamic_type_value lhs = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(lhs) || ! std::holds_alternative<double>(rhs)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << *source_map_token.lexeme
                           << "\": Operands must be numbers.";
                        throw std::runtime_error{os.str()};
                    }

                    const auto result = std::get<double>(lhs) > std::get<double>(rhs);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(result);

                    break;
                }

                case Opcode::inherit: {
                    const Dynamic_type_value maybe_parent_class = *(stack_.end() - 1);
                    if (! std::holds_alternative<GC_ptr<Class>>(maybe_parent_class)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << *source_map_token.lexeme
                           << "\": Superclass must be a class.";
                        throw std::runtime_error{os.str()};
                    }
                    const auto parent = std::get<GC_ptr<Class>>(maybe_parent_class);

                    auto child = std::get<GC_ptr<Class>>(*(stack_.end() - 2));
                    child->methods.insert(parent->methods.cbegin(), parent->methods.cend());
                    stack_.push_back(child);

                    break;
                }

                case Opcode::jump:
                case Opcode::jump_if_false:
                case Opcode::loop: {
                    const auto jump_distance_big_endian = reinterpret_cast<const std::uint16_t&>(*bytecode_iter);
                    bytecode_iter += 2;
                    const auto jump_distance = boost::endian::big_to_native(jump_distance_big_endian);

                    switch (opcode) {
                        default:
                            throw std::logic_error{"Unreachable."};

                        case Opcode::jump:
                            bytecode_iter += jump_distance;
                            break;

                        case Opcode::jump_if_false:
                            if (! std::visit(Is_truthy_visitor{}, stack_.back())) {
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
                    const Dynamic_type_value rhs = *(stack_.cend() - 1);
                    const Dynamic_type_value lhs = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(lhs) || ! std::holds_alternative<double>(rhs)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << *source_map_token.lexeme
                           << "\": Operands must be numbers.";
                        throw std::runtime_error{os.str()};
                    }

                    const auto result = std::get<double>(lhs) < std::get<double>(rhs);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(result);

                    break;
                }

                case Opcode::method: {
                    const auto method_name_constant_index = *bytecode_iter++;
                    const auto method_name = std::get<GC_ptr<const std::string>>(constants[method_name_constant_index]);
                    const auto closure = std::get<GC_ptr<Closure>>(*(stack_.cend() - 1));
                    auto klass = std::get<GC_ptr<Class>>(*(stack_.end() - 2));

                    klass->methods[method_name] = closure;
                    stack_.pop_back();

                    break;
                }

                case Opcode::multiply: {
                    const Dynamic_type_value rhs = *(stack_.cend() - 1);
                    const Dynamic_type_value lhs = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(lhs) || ! std::holds_alternative<double>(rhs)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << *source_map_token.lexeme
                           << "\": Operands must be numbers.";
                        throw std::runtime_error{os.str()};
                    }

                    const auto result = std::get<double>(lhs) * std::get<double>(rhs);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(result);

                    break;
                }

                case Opcode::negate: {
                    const Dynamic_type_value value = stack_.back();

                    if (! std::holds_alternative<double>(value)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << *source_map_token.lexeme
                           << "\": Operand must be a number.";
                        throw std::runtime_error{os.str()};
                    }

                    const auto negated_value = -std::get<double>(value);
                    stack_.pop_back();
                    stack_.push_back(negated_value);

                    break;
                }

                case Opcode::nil: {
                    stack_.push_back(nullptr);
                    break;
                }

                case Opcode::not_: {
                    const auto negated_value = ! std::visit(Is_truthy_visitor{}, stack_.back());
                    stack_.pop_back();
                    stack_.push_back(negated_value);

                    break;
                }

                case Opcode::pop: {
                    stack_.pop_back();
                    break;
                }

                case Opcode::print: {
                    os_ << stack_.back() << '\n';
                    stack_.pop_back();

                    break;
                }

                case Opcode::return_: {
                    for (GC_ptr<Upvalue> upvalue : open_upvalues) {
                        upvalue->close();
                    }
                    stack_.erase(stack_.cbegin() + stack_begin_index, stack_.cend() - 1);

                    return;
                }

                case Opcode::set_global: {
                    const auto variable_name_constant_index = *bytecode_iter++;
                    const auto variable_name = std::get<GC_ptr<const std::string>>(constants[variable_name_constant_index]);

                    const auto global_iter = globals_.find(variable_name);
                    if (global_iter == globals_.cend()) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Undefined variable \"" + *variable_name + "\"."};
                    }
                    global_iter->second = stack_.back();

                    break;
                }

                case Opcode::set_local: {
                    const auto local_stack_index = *bytecode_iter++;
                    stack_[stack_begin_index + local_stack_index] = stack_.back();

                    break;
                }

                case Opcode::set_property: {
                    const auto field_name_constant_index = *bytecode_iter++;
                    const auto field_name = std::get<GC_ptr<const std::string>>(constants[field_name_constant_index]);

                    const Dynamic_type_value maybe_instance = *(stack_.cend() - 1);
                    if (! std::holds_alternative<GC_ptr<Instance>>(maybe_instance)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << *source_map_token.lexeme
                           << "\": Only instances have fields.";
                        throw std::runtime_error{os.str()};
                    }
                    auto instance = std::get<GC_ptr<Instance>>(maybe_instance);

                    instance->fields[field_name] = *(stack_.cend() - 2);
                    stack_.pop_back();

                    break;
                }

                case Opcode::set_upvalue: {
                    const auto upvalue_index = *bytecode_iter++;
                    upvalues[upvalue_index]->value() = stack_.back();

                    break;
                }

                case Opcode::subtract: {
                    const Dynamic_type_value rhs = *(stack_.cend() - 1);
                    const Dynamic_type_value lhs = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(lhs) || ! std::holds_alternative<double>(rhs)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << *source_map_token.lexeme
                           << "\": Operands must be numbers.";
                        throw std::runtime_error{os.str()};
                    }

                    const auto result = std::get<double>(lhs) - std::get<double>(rhs);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(result);

                    break;
                }

                case Opcode::true_: {
                    stack_.push_back(true);
                    break;
                }
            }

            // Run the garbage collector only occassionally based on how fast the allocation size grows.
            // 4K is (semi) arbitrarily chosen. Could be tuned with performance testing.
            if (gc_heap_.size() - gc_heap_last_collect_size_ > 4096) {
                if (debug_) {
                    os_ << "# Collecting garbage: " << gc_heap_.size() << " bytes -> ";
                }

                gc_heap_.collect_garbage();
                gc_heap_last_collect_size_ = gc_heap_.size();

                if (debug_) {
                    os_ << gc_heap_last_collect_size_ << '\n';
                }
            }

            if (debug_) {
                os_ << "# Stack:\n";
                for (auto stack_iter = stack_.crbegin(); stack_iter != stack_.crend(); ++stack_iter) {
                    const auto stack_index = stack_iter.base() - 1 - stack_.cbegin();
                    os_ << std::setw(5) << std::setfill(' ') << std::right << stack_index << " : " << *stack_iter << '\n';
                }
                os_ << '\n';
            }
        }
    }
}
