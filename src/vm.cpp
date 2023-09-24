#include "vm.hpp"

#include <chrono>
#include <iomanip>

#include <boost/endian/conversion.hpp>
#include <gsl/gsl>

#include "object.hpp"

// Not exported (internal linkage)
namespace
{
    using namespace motts::lox;

    Dynamic_type_value clock_native(std::span<Dynamic_type_value>)
    {
        const auto now_time_point = std::chrono::system_clock::now().time_since_epoch();
        const auto now_seconds = std::chrono::duration_cast<std::chrono::seconds>(now_time_point).count();
        return gsl::narrow<double>(now_seconds);
    }
}

namespace motts { namespace lox
{
    VM::VM(GC_heap& gc_heap, std::ostream& os, bool debug)
        : debug_ {debug},
          os_ {os},
          gc_heap_ {gc_heap}
    {
        gc_heap_.on_mark_roots.push_back([&] {
            for (const auto& call_frame : call_frames_) {
                mark(gc_heap_, call_frame.closure);
                for (const auto& root_value : call_frame.chunk.constants()) {
                    std::visit(Mark_objects_visitor{gc_heap_}, root_value);
                }
            }

            for (const auto& root_value : stack_) {
                std::visit(Mark_objects_visitor{gc_heap_}, root_value);
            }

            for (const auto& [_, root_value] : globals_) {
                std::visit(Mark_objects_visitor{gc_heap_}, root_value);
            }
        });

        globals_["clock"] = gc_heap_.make<Native_fn>({clock_native});
    }

    VM::~VM()
    {
        gc_heap_.on_mark_roots.pop_back();
    }

    void VM::run(const Chunk& chunk)
    {
        const auto root_script_fn = gc_heap_.make<Closure>(gc_heap_.make<Function>({"", 0, chunk}));
        call_frames_.push_back({root_script_fn, chunk, chunk.bytecode().cbegin(), 0});
        const auto _ = gsl::finally([&] {
            call_frames_.pop_back();
        });

        if (debug_) {
            os_ << "\n# Running chunk:\n\n" << call_frames_.back().chunk << '\n';
        }

        while (call_frames_.back().bytecode_iter != call_frames_.back().chunk.bytecode().cend()) {
            const auto bytecode_index = call_frames_.back().bytecode_iter - call_frames_.back().chunk.bytecode().cbegin();
            const auto& source_map_token = call_frames_.back().chunk.source_map_tokens().at(bytecode_index);

            const auto opcode = static_cast<Opcode>(*call_frames_.back().bytecode_iter);
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

                    if (std::holds_alternative<double>(lhs) && std::holds_alternative<double>(rhs)) {
                        const auto result = std::get<double>(lhs) + std::get<double>(rhs);
                        stack_.erase(stack_.cend() - 2, stack_.cend());
                        stack_.push_back(result);
                    } else if (std::holds_alternative<std::string>(lhs) && std::holds_alternative<std::string>(rhs)) {
                        auto result = std::get<std::string>(lhs) + std::get<std::string>(rhs);
                        stack_.erase(stack_.cend() - 2, stack_.cend());
                        stack_.push_back(std::move(result));
                    } else {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": Operands must be two numbers or two strings.";
                        throw std::runtime_error{os.str()};
                    }

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::call: {
                    const auto arg_count = *(call_frames_.back().bytecode_iter + 1);
                    auto& maybe_callable = *(stack_.end() - arg_count - 1);

                   call_frames_.back().bytecode_iter += 2;

                    if (std::holds_alternative<GC_ptr<Closure>>(maybe_callable)) {
                        const auto closure = std::get<GC_ptr<Closure>>(maybe_callable);

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
                    } else if (std::holds_alternative<GC_ptr<Class>>(maybe_callable)) {
                        const auto klass = std::get<GC_ptr<Class>>(maybe_callable);
                        const auto maybe_init_iter = klass->methods.find("init");
                        const auto expected_arity = maybe_init_iter != klass->methods.cend() ? maybe_init_iter->second->function->arity : 0;

                        if (arg_count != expected_arity) {
                            std::ostringstream os;
                            os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": "
                                << "Expected " << expected_arity << " arguments but got " << static_cast<int>(arg_count) << '.';
                            throw std::runtime_error{os.str()};
                        }

                        // If there's no init method, then we'd pop the class and push the instance,
                        // so assigning the instance into the class slot has the same effect.
                        // But if there's an init method, then we need to prepare the stack like a bound method,
                        // which means putting the "this" instance in the class slot before the arguments.
                        // Either way, the instance ends up in the same slot where the class was.
                        maybe_callable = gc_heap_.make<Instance>({klass});

                        if (maybe_init_iter != klass->methods.cend()) {
                            call_frames_.push_back({
                                maybe_init_iter->second,
                                maybe_init_iter->second->function->chunk,
                                maybe_init_iter->second->function->chunk.bytecode().cbegin(),
                                stack_.size() - arg_count - 1
                            });
                        }
                    } else if (std::holds_alternative<GC_ptr<Bound_method>>(maybe_callable)) {
                        const auto bound_method = std::get<GC_ptr<Bound_method>>(maybe_callable);

                        if (bound_method->method->function->arity != arg_count) {
                            std::ostringstream os;
                            os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": "
                                << "Expected " << bound_method->method->function->arity << " arguments but got " << static_cast<int>(arg_count) << '.';
                            throw std::runtime_error{os.str()};
                        }

                        call_frames_.push_back({
                            bound_method->method,
                            bound_method->method->function->chunk,
                            bound_method->method->function->chunk.bytecode().cbegin(),
                            stack_.size() - arg_count - 1
                        });

                        // Replace function at call frame stack slot 0 with "this" instance at slot 0.
                        maybe_callable = bound_method->instance;
                    } else if (std::holds_alternative<GC_ptr<Native_fn>>(maybe_callable)) {
                        auto return_value = std::get<GC_ptr<Native_fn>>(maybe_callable)->fn({stack_.end() - arg_count, stack_.end()});
                        stack_.erase(stack_.end() - arg_count - 1, stack_.end());
                        stack_.push_back(std::move(return_value));
                    } else {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": "
                            << "Can only call functions and classes.";
                        throw std::runtime_error{os.str()};
                    }

                    break;
                }

                case Opcode::class_: {
                    const auto class_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& class_name = std::get<std::string>(call_frames_.back().chunk.constants().at(class_name_constant_index));
                    stack_.push_back(gc_heap_.make<Class>({class_name}));

                    call_frames_.back().bytecode_iter += 2;

                    break;
                }

                case Opcode::close_upvalue: {
                    if (
                        ! call_frames_.back().closure->open_upvalues.empty()
                        && call_frames_.back().closure->open_upvalues.back()->stack_index == stack_.size() - 1
                    ) {
                        call_frames_.back().closure->open_upvalues.back()->close();
                        call_frames_.back().closure->open_upvalues.pop_back();
                    }
                    stack_.pop_back();

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::closure: {
                    const auto fn_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto function = std::get<GC_ptr<Function>>(call_frames_.back().chunk.constants().at(fn_constant_index));
                    auto new_closure = gc_heap_.make<Closure>({function});
                    stack_.push_back(new_closure);

                    const auto n_upvalues = *(call_frames_.back().bytecode_iter + 2);
                    for (auto n_upvalue = 0; n_upvalue != n_upvalues; ++n_upvalue) {
                        const auto is_direct_capture = *(call_frames_.back().bytecode_iter + 3 + 2 * n_upvalue);
                        const auto enclosing_index = *(call_frames_.back().bytecode_iter + 3 + 2 * n_upvalue + 1);

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

                    call_frames_.back().bytecode_iter += 3 + 2 * n_upvalues;

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

                    if (! std::holds_alternative<double>(lhs) || ! std::holds_alternative<double>(rhs)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": Operands must be numbers.";
                        throw std::runtime_error{os.str()};
                    }

                    const auto result = std::get<double>(lhs) / std::get<double>(rhs);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(result);

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::equal: {
                    const auto& rhs = *(stack_.cend() - 1);
                    const auto& lhs = *(stack_.cend() - 2);

                    const auto result = lhs == rhs;
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(result);

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::false_: {
                    stack_.push_back(false);
                    ++call_frames_.back().bytecode_iter;
                    break;
                }

                case Opcode::define_global: {
                    const auto variable_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& variable_name = std::get<std::string>(call_frames_.back().chunk.constants().at(variable_name_constant_index));
                    globals_[variable_name] = stack_.back();
                    stack_.pop_back();

                    call_frames_.back().bytecode_iter += 2;

                    break;
                }

                case Opcode::get_global: {
                    const auto variable_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& variable_name = std::get<std::string>(call_frames_.back().chunk.constants().at(variable_name_constant_index));

                    const auto global_iter = globals_.find(variable_name);
                    if (global_iter == globals_.cend()) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Undefined variable \"" + variable_name + "\"."
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

                case Opcode::get_property: {
                    const auto field_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& field_name = std::get<std::string>(call_frames_.back().chunk.constants().at(field_name_constant_index));
                    if (! std::holds_alternative<GC_ptr<Instance>>(stack_.back())) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": Only instances have fields.";
                        throw std::runtime_error{os.str()};
                    }
                    const auto instance = std::get<GC_ptr<Instance>>(stack_.back());

                    call_frames_.back().bytecode_iter += 2;

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
                        "[Line " + std::to_string(source_map_token.line) + "] Error: Undefined property \"" + field_name + "\"."
                    };
                }

                case Opcode::get_super: {
                    const auto method_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& method_name = std::get<std::string>(call_frames_.back().chunk.constants().at(method_name_constant_index));
                    const auto superclass = std::get<GC_ptr<Class>>(*(stack_.cend() - 1));
                    const auto instance = std::get<GC_ptr<Instance>>(*(stack_.cend() - 2));

                    const auto maybe_method_iter = superclass->methods.find(method_name);
                    if (maybe_method_iter == superclass->methods.cend()) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Undefined property \"" + method_name + "\"."
                        };
                    }

                    const auto new_bound_method = gc_heap_.make<Bound_method>({instance, maybe_method_iter->second});
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(new_bound_method);

                    call_frames_.back().bytecode_iter += 2;

                    break;
                }

                case Opcode::get_upvalue: {
                    const auto upvalue_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& upvalue = call_frames_.back().closure->upvalues.at(upvalue_index);
                    stack_.push_back(upvalue->value());

                    call_frames_.back().bytecode_iter += 2;

                    break;
                }

                case Opcode::greater: {
                    const auto& rhs = *(stack_.cend() - 1);
                    const auto& lhs = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(lhs) || ! std::holds_alternative<double>(rhs)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": Operands must be numbers.";
                        throw std::runtime_error{os.str()};
                    }

                    const auto result = std::get<double>(lhs) > std::get<double>(rhs);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(result);

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::inherit: {
                    const auto& maybe_parent_class = *(stack_.end() - 1);
                    if (! std::holds_alternative<GC_ptr<Class>>(maybe_parent_class)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": Superclass must be a class.";
                        throw std::runtime_error{os.str()};
                    }
                    const auto parent = std::get<GC_ptr<Class>>(maybe_parent_class);
                    auto child = std::get<GC_ptr<Class>>(*(stack_.end() - 2));

                    child->methods.insert(parent->methods.cbegin(), parent->methods.cend());
                    stack_.push_back(child);

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
                            if (! std::visit(Is_truthy_visitor{}, stack_.back())) {
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

                    if (! std::holds_alternative<double>(lhs) || ! std::holds_alternative<double>(rhs)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": Operands must be numbers.";
                        throw std::runtime_error{os.str()};
                    }

                    const auto result = std::get<double>(lhs) < std::get<double>(rhs);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(result);

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::method: {
                    const auto method_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& method_name = std::get<std::string>(call_frames_.back().chunk.constants().at(method_name_constant_index));
                    const auto closure = std::get<GC_ptr<Closure>>(*(stack_.cend() - 1));
                    auto klass = std::get<GC_ptr<Class>>(*(stack_.end() - 2));

                    klass->methods[method_name] = closure;
                    stack_.pop_back();

                    call_frames_.back().bytecode_iter += 2;

                    break;
                }

                case Opcode::multiply: {
                    const auto& rhs = *(stack_.cend() - 1);
                    const auto& lhs = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(lhs) || ! std::holds_alternative<double>(rhs)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": Operands must be numbers.";
                        throw std::runtime_error{os.str()};
                    }

                    const auto result = std::get<double>(lhs) * std::get<double>(rhs);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(result);

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::negate: {
                    if (! std::holds_alternative<double>(stack_.back())) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": Operand must be a number.";
                        throw std::runtime_error{os.str()};
                    }

                    const auto negated_value = - std::get<double>(stack_.back());
                    stack_.pop_back();
                    stack_.push_back(negated_value);

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::nil: {
                    stack_.push_back(nullptr);
                    ++call_frames_.back().bytecode_iter;
                    break;
                }

                case Opcode::not_: {
                    const auto negated_value = ! std::visit(Is_truthy_visitor{}, stack_.back());
                    stack_.pop_back();
                    stack_.push_back(negated_value);

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
                    const auto variable_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& variable_name = std::get<std::string>(call_frames_.back().chunk.constants().at(variable_name_constant_index));

                    const auto global_iter = globals_.find(variable_name);
                    if (global_iter == globals_.end()) {
                        throw std::runtime_error{
                            "[Line " + std::to_string(source_map_token.line) + "] Error: Undefined variable \"" + variable_name + "\"."
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

                case Opcode::set_property: {
                    const auto field_name_constant_index = *(call_frames_.back().bytecode_iter + 1);
                    const auto& field_name = std::get<std::string>(call_frames_.back().chunk.constants().at(field_name_constant_index));
                    if (! std::holds_alternative<GC_ptr<Instance>>(stack_.back())) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": Only instances have fields.";
                        throw std::runtime_error{os.str()};
                    }
                    auto instance = std::get<GC_ptr<Instance>>(stack_.back());

                    instance->fields[field_name] = *(stack_.cend() - 2);
                    stack_.pop_back();

                    call_frames_.back().bytecode_iter += 2;

                    break;
                }

                case Opcode::set_upvalue: {
                    const auto upvalue_index = *(call_frames_.back().bytecode_iter + 1);
                    auto& upvalue = call_frames_.back().closure->upvalues.at(upvalue_index);
                    upvalue->value() = stack_.back();

                    call_frames_.back().bytecode_iter += 2;

                    break;
                }

                case Opcode::subtract: {
                    const auto& rhs = *(stack_.cend() - 1);
                    const auto& lhs = *(stack_.cend() - 2);

                    if (! std::holds_alternative<double>(lhs) || ! std::holds_alternative<double>(rhs)) {
                        std::ostringstream os;
                        os << "[Line " << source_map_token.line << "] Error at \"" << source_map_token.lexeme << "\": Operands must be numbers.";
                        throw std::runtime_error{os.str()};
                    }

                    const auto result = std::get<double>(lhs) - std::get<double>(rhs);
                    stack_.erase(stack_.cend() - 2, stack_.cend());
                    stack_.push_back(result);

                    ++call_frames_.back().bytecode_iter;

                    break;
                }

                case Opcode::true_: {
                    stack_.push_back(true);
                    ++call_frames_.back().bytecode_iter;
                    break;
                }
            }

            // Run the garbage collector only occassionally based on how fast the allocation size grows.
            // 4K is (semi) arbitrarily chosen. Could be tuned with performance testing.
            if (gc_heap_.size() - gc_heap_last_collect_size_ > 4096) {
                if (debug_) {
                    os_ << "DEBUG: Collect garbage: " << gc_heap_.size() << " bytes -> ";
                }

                gc_heap_.collect_garbage();
                gc_heap_last_collect_size_ = gc_heap_.size();

                if (debug_) {
                    os_ << gc_heap_last_collect_size_ << '\n';
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
    }
}}
