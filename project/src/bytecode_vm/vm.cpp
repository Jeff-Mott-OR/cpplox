#include "vm.hpp"

#include <chrono>
#include <iostream>

#include "debug.hpp"

// Not exported (internal linkage)
namespace {
    // Allow the internal linkage section to access names
    using namespace motts::lox;

    struct VM_runtime_error : std::runtime_error {
        VM_runtime_error(const Token& debug_token, const std::string& message) :
            std::runtime_error{
                "[Line " + std::to_string(debug_token.line) + "] "
                "Error at " + (
                    debug_token.type != Token_type::eof ? ('\'' + gsl::to_string(debug_token.lexeme) + '\'') : "end"
                ) + ": " + message
            }
        {}
    };

    auto clock_native(const std::vector<Value>& /*args*/) {
        return Value{
            gsl::narrow<double>(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count()
            )
        };
    }

    struct Is_string_visitor : boost::static_visitor<bool> {
        auto operator()(const GC_ptr<std::string>&) const {
            return true;
        }

        template<typename T>
            auto operator()(const T&) const {
               return false;
            }
    };

    struct Is_number_visitor : boost::static_visitor<bool> {
        auto operator()(double) const {
            return true;
        }

        template<typename T>
            auto operator()(const T&) const {
               return false;
            }
    };

    struct Is_truthy_visitor : boost::static_visitor<bool> {
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
    struct VM::Push_stack_frame_visitor : boost::static_visitor<void> {
        VM& vm;
        std::uint8_t arg_count;

        Push_stack_frame_visitor(VM& vm_arg, std::uint8_t arg_count_arg) :
            vm {vm_arg},
            arg_count {arg_count_arg}
        {}

        auto operator()(const GC_ptr<Native_fn>& native_fn) const {
            const auto return_value = native_fn->fn({vm.stack_.end() - arg_count, vm.stack_.end()});
            vm.stack_.erase(vm.stack_.end() - arg_count - 1, vm.stack_.end());
            vm.stack_.push_back(return_value);
        }

        auto operator()(const GC_ptr<Closure>& closure) const {
            if (closure->function->arity != arg_count) {
                const auto arg_count_opcode_offset = vm.frames_.back().opcode_iter - 1 - vm.frames_.back().closure->function->chunk.opcodes.cbegin();
                const auto debug_token = vm.frames_.back().closure->function->chunk.tokens.at(arg_count_opcode_offset);
                throw VM_runtime_error{debug_token, "Expected " + std::to_string(closure->function->arity) + " arguments but got " + std::to_string(arg_count) + "."};
            }

            vm.frames_.push_back({closure, closure->function->chunk.opcodes.cbegin(), vm.stack_.size() - arg_count - 1});
        }

        auto operator()(const GC_ptr<Class>& klass) const {
            // HACK? We want the stack_ to look like we invoked a method on an instance,
            // so replace the invoked class with the new "this" instance to get the same effect
            *(vm.stack_.end() - arg_count - 1) = Value{vm.gc_heap_.make(Instance{klass})};

            const auto initializer_iter = klass->methods.find(vm.interned_strings_.get("init"));
            if (initializer_iter != klass->methods.cend()) {
                const auto initializer_closure = boost::get<GC_ptr<Closure>>(initializer_iter->second.variant);

                if (initializer_closure->function->arity != arg_count) {
                    const auto opcode_offset = vm.frames_.back().opcode_iter - vm.frames_.back().closure->function->chunk.opcodes.begin();
                    const auto debug_token = vm.frames_.back().closure->function->chunk.tokens.at(opcode_offset - 1);
                    throw VM_runtime_error{debug_token, "Expected " + std::to_string(initializer_closure->function->arity) + " arguments but got " + std::to_string(arg_count) + "."};
                }

                vm.frames_.push_back({initializer_closure, initializer_closure->function->chunk.opcodes.begin(), vm.stack_.size() - arg_count - 1});

                return;
            }

            if (arg_count > 0) {
                const auto opcode_offset = vm.frames_.back().opcode_iter - vm.frames_.back().closure->function->chunk.opcodes.begin();
                const auto debug_token = vm.frames_.back().closure->function->chunk.tokens.at(opcode_offset - 1);
                throw VM_runtime_error{debug_token, "Expected 0 arguments but got " + std::to_string(arg_count) + "."};
            }
        }

        auto operator()(const GC_ptr<Bound_method>& bound) const {
            // HACK? Most times, the instance will already be just before the args,
            // but if a bound instance is assigned and later invoked,
            // the stack_ will have a function there instead of the instance
            *(vm.stack_.end() - arg_count - 1) = bound->this_;

            if (arg_count != bound->method->function->arity) {
                throw std::runtime_error{"Expected " + std::to_string(bound->method->function->arity) + " arguments but got " + std::to_string(arg_count) + "."};
            }

            vm.frames_.push_back({bound->method, bound->method->function->chunk.opcodes.begin(), vm.stack_.size() - arg_count - 1});
        }

        // All other value types cannot be called
        template<typename T>
            void operator()(const T&) const {
                throw std::runtime_error{"Can only call functions and classes."};
            }
    };

    VM::VM(GC_heap& gc_heap, Interned_strings& interned_strings) :
        gc_heap_ {gc_heap},
        interned_strings_ {interned_strings}
    {
        gc_heap_.on_mark_roots.push_back([&] () {
            for (const auto& global : globals_) {
                gc_heap_.mark(global.first);
                boost::apply_visitor(Mark_value_visitor{gc_heap}, global.second.variant);
            }

            for (const auto& value : stack_) {
                boost::apply_visitor(Mark_value_visitor{gc_heap}, value.variant);
            }

            for (const auto& frame : frames_) {
                gc_heap_.mark(frame.closure);
            }

            for (const auto& upvalue : open_upvalues_) {
                gc_heap_.mark(upvalue);
            }
        });

        globals_[interned_strings_.get("clock")] = Value{gc_heap_.make(Native_fn{clock_native})};
    }

    void VM::run(const GC_ptr<Closure>& script_closure, bool debug) {
        frames_.push_back({script_closure, script_closure->function->chunk.opcodes.begin(), stack_.size()});
        stack_.push_back({script_closure});

        if (debug) {
            disassemble(std::cout, script_closure->function);
            std::cout << '\n' << stack_ << '\n';
        }

        while (true) {
            if (gc_heap_.size() - gc_heap_last_collect_size_ > 4096) {
                if (debug) {
                    std::cout << "DEBUG: Collect garbage: " << gc_heap_.size() << " bytes -> ";
                }

                gc_heap_.collect_garbage();
                gc_heap_last_collect_size_ = gc_heap_.size();

                if (debug) {
                    std::cout << gc_heap_.size() << '\n';
                }
            }

            if (debug) {
                disassemble_instruction(std::cout, frames_.back().opcode_iter, frames_.back().closure->function->chunk);
            }

            switch (static_cast<Opcode>(*frames_.back().opcode_iter++)) {
                case Opcode::constant: {
                    const auto constant_index = *frames_.back().opcode_iter++;
                    stack_.push_back(frames_.back().closure->function->chunk.constants.at(constant_index));
                    break;
                }

                case Opcode::nil: {
                    stack_.push_back({nullptr});
                    break;
                }

                case Opcode::true_: {
                    stack_.push_back({true});
                    break;
                }

                case Opcode::false_: {
                    stack_.push_back({false});
                    break;
                }

                case Opcode::pop: {
                    stack_.pop_back();
                    break;
                }

                case Opcode::get_local: {
                    const auto local_index = *frames_.back().opcode_iter++;
                    stack_.push_back(stack_.at(frames_.back().stack_begin_index + local_index));
                    break;
                }

                case Opcode::set_local: {
                    const auto local_index = *frames_.back().opcode_iter++;
                    stack_.at(frames_.back().stack_begin_index + local_index) = stack_.back();
                    break;
                }

                case Opcode::get_global: {
                    const auto global_name_constant_index = *frames_.back().opcode_iter++;
                    const auto global_name = boost::get<GC_ptr<std::string>>(
                        frames_.back().closure->function->chunk.constants.at(global_name_constant_index).variant
                    );
                    const auto global_value_iter = globals_.find(global_name);
                    if (global_value_iter == globals_.cend()) {
                        throw std::runtime_error{"Undefined variable '" + *global_name + "'."};
                    }
                    stack_.push_back(global_value_iter->second);
                    break;
                }

                case Opcode::define_global: {
                    const auto global_name_constant_index = *frames_.back().opcode_iter++;
                    const auto global_name = boost::get<GC_ptr<std::string>>(
                        frames_.back().closure->function->chunk.constants.at(global_name_constant_index).variant
                    );
                    globals_[global_name] = stack_.back();
                    stack_.pop_back();
                    break;
                }

                case Opcode::set_global: {
                    const auto global_name_constant_index = *frames_.back().opcode_iter++;
                    const auto global_name = boost::get<GC_ptr<std::string>>(
                        frames_.back().closure->function->chunk.constants.at(global_name_constant_index).variant
                    );
                    const auto global_value_iter = globals_.find(global_name);
                    if (global_value_iter == globals_.cend()) {
                        throw std::runtime_error{"Undefined variable '" + *global_name + "'."};
                    }
                    global_value_iter->second = stack_.back();
                    break;
                }

                case Opcode::get_upvalue: {
                    const auto upvalue_index = *frames_.back().opcode_iter++;
                    stack_.push_back(frames_.back().closure->upvalues.at(upvalue_index)->value());
                    break;
                }

                case Opcode::set_upvalue: {
                    const auto upvalue_index = *frames_.back().opcode_iter++;
                    frames_.back().closure->upvalues.at(upvalue_index)->value() = stack_.back();
                    break;
                }

                case Opcode::get_property: {
                    const auto instance = ([&] () {
                        try {
                            return boost::get<GC_ptr<Instance>>(stack_.back().variant);
                        } catch (const boost::bad_get&) {
                            const auto opcode_offset = frames_.back().opcode_iter - frames_.back().closure->function->chunk.opcodes.begin();
                            const auto debug_token = frames_.back().closure->function->chunk.tokens.at(opcode_offset);
                            throw VM_runtime_error{debug_token, "Only instances have properties."};
                        }
                    })();

                    const auto property_name_constant_index = *frames_.back().opcode_iter++;
                    const auto property_name = boost::get<GC_ptr<std::string>>(
                        frames_.back().closure->function->chunk.constants.at(property_name_constant_index).variant
                    );

                    // Fields get first dibs
                    const auto property_value_iter = instance->fields.find(property_name);
                    if (property_value_iter != instance->fields.cend()) {
                        stack_.pop_back();
                        stack_.push_back(property_value_iter->second);
                        break;
                    }

                    // Methods second
                    const auto method_iter = instance->klass->methods.find(property_name);
                    if (method_iter != instance->klass->methods.cend()) {
                        const auto bound_method = gc_heap_.make(
                            Bound_method{stack_.back(), boost::get<GC_ptr<Closure>>(method_iter->second.variant)}
                        );
                        stack_.pop_back();
                        stack_.push_back({bound_method});
                        break;
                    }

                    throw std::runtime_error{"Undefined property '" + *property_name + "'."};
                }

                case Opcode::set_property: {
                    const auto instance = ([&] () {
                        try {
                            return boost::get<GC_ptr<Instance>>((stack_.end()-2)->variant);
                        } catch (const boost::bad_get&) {
                            const auto opcode_offset = frames_.back().opcode_iter - frames_.back().closure->function->chunk.opcodes.begin();
                            const auto debug_token = frames_.back().closure->function->chunk.tokens.at(opcode_offset);
                            throw VM_runtime_error{debug_token, "Only instances have fields."};
                        }
                    })();

                    const auto property_name_constant_index = *frames_.back().opcode_iter++;
                    const auto property_name = boost::get<GC_ptr<std::string>>(
                        frames_.back().closure->function->chunk.constants.at(property_name_constant_index).variant
                    );
                    instance->fields[property_name] = stack_.back();

                    stack_.pop_back();
                    stack_.pop_back();
                    stack_.push_back(instance->fields[property_name]);

                    break;
                }

                case Opcode::get_super: {
                    const auto method_name_constant_index = *frames_.back().opcode_iter++;
                    const auto method_name = boost::get<GC_ptr<std::string>>(
                        frames_.back().closure->function->chunk.constants.at(method_name_constant_index).variant
                    );

                    const auto superclass = boost::get<GC_ptr<Class>>(stack_.back().variant);
                    stack_.pop_back();

                    const auto method_iter = superclass->methods.find(method_name);
                    if (method_iter == superclass->methods.cend()) {
                        throw std::runtime_error{"Undefined property '" + *method_name + "'."};
                    }
                    const auto bound_method = gc_heap_.make(
                        Bound_method{stack_.back(), boost::get<GC_ptr<Closure>>(method_iter->second.variant)}
                    );
                    stack_.pop_back();
                    stack_.push_back({bound_method});

                    break;
                }

                case Opcode::equal: {
                    const auto b = stack_.back();
                    stack_.pop_back();
                    const auto a = stack_.back();
                    stack_.pop_back();

                    stack_.push_back({a == b});

                    break;
                }

                case Opcode::greater: {
                    try {
                        const auto b = boost::get<double>(stack_.back().variant);
                        stack_.pop_back();
                        const auto a = boost::get<double>(stack_.back().variant);
                        stack_.pop_back();

                        stack_.push_back({a > b});
                    } catch (const boost::bad_get&) {
                        const auto opcode_offset = frames_.back().opcode_iter - 1 - frames_.back().closure->function->chunk.opcodes.begin();
                        const auto debug_token = frames_.back().closure->function->chunk.tokens.at(opcode_offset);
                        throw VM_runtime_error{debug_token, "Operands must be numbers."};
                    }

                    break;
                }

                case Opcode::less: {
                    try {
                        const auto b = boost::get<double>(stack_.back().variant);
                        stack_.pop_back();
                        const auto a = boost::get<double>(stack_.back().variant);
                        stack_.pop_back();

                        stack_.push_back({a < b});
                    } catch (const boost::bad_get&) {
                        const auto opcode_offset = frames_.back().opcode_iter - 1 - frames_.back().closure->function->chunk.opcodes.begin();
                        const auto token = frames_.back().closure->function->chunk.tokens.at(opcode_offset);
                        throw std::runtime_error{"[Line " + std::to_string(token.line) + "] Error at '" + gsl::to_string(token.lexeme) + "': Operands must be numbers."};
                    }

                    break;
                }

                case Opcode::add: {
                    if (
                        boost::apply_visitor(Is_string_visitor{}, (stack_.end()-1)->variant) &&
                        boost::apply_visitor(Is_string_visitor{}, (stack_.end()-2)->variant)
                    ) {
                        const auto b = boost::get<GC_ptr<std::string>>(stack_.back().variant);
                        stack_.pop_back();
                        const auto a = boost::get<GC_ptr<std::string>>(stack_.back().variant);
                        stack_.pop_back();

                        stack_.push_back({interned_strings_.get(*a + *b)});

                        break;
                    }

                    if (
                        boost::apply_visitor(Is_number_visitor{}, (stack_.end()-1)->variant) &&
                        boost::apply_visitor(Is_number_visitor{}, (stack_.end()-2)->variant)
                    ) {
                        const auto b = boost::get<double>(stack_.back().variant);
                        stack_.pop_back();
                        const auto a = boost::get<double>(stack_.back().variant);
                        stack_.pop_back();

                        stack_.push_back({a + b});

                        break;
                    }

                    throw std::runtime_error{"Operands must be two numbers or two strings."};
                }

                case Opcode::subtract: {
                    try {
                        const auto b = boost::get<double>(stack_.back().variant);
                        stack_.pop_back();
                        const auto a = boost::get<double>(stack_.back().variant);
                        stack_.pop_back();

                        stack_.push_back({a - b});
                    } catch (const boost::bad_get&) {
                        const auto opcode_offset = frames_.back().opcode_iter - 1 - frames_.back().closure->function->chunk.opcodes.begin();
                        const auto debug_token = frames_.back().closure->function->chunk.tokens.at(opcode_offset);
                        throw VM_runtime_error{debug_token, "Operands must be numbers."};
                    }

                    break;
                }

                case Opcode::multiply: {
                    try {
                        const auto b = boost::get<double>(stack_.back().variant);
                        stack_.pop_back();
                        const auto a = boost::get<double>(stack_.back().variant);
                        stack_.pop_back();

                        stack_.push_back({a * b});
                    } catch (const boost::bad_get&) {
                        const auto opcode_offset = frames_.back().opcode_iter - 1 - frames_.back().closure->function->chunk.opcodes.begin();
                        const auto debug_token = frames_.back().closure->function->chunk.tokens.at(opcode_offset);
                        throw VM_runtime_error{debug_token, "Operands must be numbers."};
                    }

                    break;
                }

                case Opcode::divide: {
                    try {
                        const auto b = boost::get<double>(stack_.back().variant);
                        stack_.pop_back();
                        const auto a = boost::get<double>(stack_.back().variant);
                        stack_.pop_back();

                        stack_.push_back({a / b});
                    } catch (const boost::bad_get&) {
                        const auto opcode_offset = frames_.back().opcode_iter - 1 - frames_.back().closure->function->chunk.opcodes.begin();
                        const auto debug_token = frames_.back().closure->function->chunk.tokens.at(opcode_offset);
                        throw VM_runtime_error{debug_token, "Operands must be numbers."};
                    }

                    break;
                }

                case Opcode::not_: {
                    const auto is_truthy = boost::apply_visitor(Is_truthy_visitor{}, stack_.back().variant);
                    stack_.pop_back();
                    stack_.push_back({!is_truthy});
                    break;
                }

                case Opcode::negate: {
                    try {
                        const auto value = boost::get<double>(stack_.back().variant);
                        stack_.pop_back();
                        stack_.push_back({-value});
                    } catch (const boost::bad_get&) {
                        const auto opcode_offset = frames_.back().opcode_iter - 1 - frames_.back().closure->function->chunk.opcodes.begin();
                        const auto debug_token = frames_.back().closure->function->chunk.tokens.at(opcode_offset);
                        throw VM_runtime_error{debug_token, "Operand must be a number."};
                    }

                    break;
                }

                case Opcode::print: {
                    std::cout << stack_.back() << '\n';
                    stack_.pop_back();
                    break;
                }

                case Opcode::jump: {
                    const auto jump_distance = (*frames_.back().opcode_iter << 8) | *(frames_.back().opcode_iter + 1);
                    frames_.back().opcode_iter += 2;
                    frames_.back().opcode_iter += jump_distance;
                    break;
                }

                case Opcode::jump_if_false: {
                    const auto jump_distance = (*frames_.back().opcode_iter << 8) | *(frames_.back().opcode_iter + 1);
                    frames_.back().opcode_iter += 2;
                    if ( ! boost::apply_visitor(Is_truthy_visitor{}, stack_.back().variant)) {
                        frames_.back().opcode_iter += jump_distance;
                    }
                    break;
                }

                case Opcode::loop: {
                    const auto jump_distance = (*frames_.back().opcode_iter << 8) | *(frames_.back().opcode_iter + 1);
                    frames_.back().opcode_iter += 2;
                    frames_.back().opcode_iter -= jump_distance;
                    break;
                }

                case Opcode::call: {
                    const auto arg_count = *frames_.back().opcode_iter++;
                    const auto callable_value = *(stack_.cend() - arg_count - 1);
                    boost::apply_visitor(Push_stack_frame_visitor{*this, arg_count}, callable_value.variant);
                    break;
                }

                case Opcode::invoke: {
                    const auto method_name_constant_index = *frames_.back().opcode_iter++;
                    const auto method_name = boost::get<GC_ptr<std::string>>(
                        frames_.back().closure->function->chunk.constants.at(method_name_constant_index).variant
                    );

                    const auto arg_count = *frames_.back().opcode_iter++;
                    const auto this_iter = stack_.end() - arg_count - 1;
                    const auto instance = ([&] () {
                        try {
                            return boost::get<GC_ptr<Instance>>(this_iter->variant);
                        } catch (const boost::bad_get&) {
                            throw std::runtime_error{"Only instances have methods."};
                        }
                    })();

                    // Fields get first dibs
                    // If we're invoking a field, we expect it to be a bound method
                    const auto field_iter = instance->fields.find(method_name);
                    if (field_iter != instance->fields.cend()) {
                        // Set bound method into "this" slot
                        *(stack_.end() - arg_count - 1) = field_iter->second;

                        boost::apply_visitor(Push_stack_frame_visitor{*this, arg_count}, field_iter->second.variant);
                        break;
                    }

                    const auto method_iter = instance->klass->methods.find(method_name);
                    if (method_iter == instance->klass->methods.cend()) {
                        throw std::runtime_error{"Undefined property '" + *method_name + "'."};
                    }
                    const auto method_closure = boost::get<GC_ptr<Closure>>(method_iter->second.variant);
                    if (method_closure->function->arity != arg_count) {
                        const auto opcode_offset = frames_.back().opcode_iter - frames_.back().closure->function->chunk.opcodes.begin();
                        const auto debug_token = frames_.back().closure->function->chunk.tokens.at(opcode_offset - 1);
                        throw VM_runtime_error{debug_token, "Expected " + std::to_string(method_closure->function->arity) + " arguments but got " + std::to_string(arg_count) + "."};
                    }

                    frames_.push_back({method_closure, method_closure->function->chunk.opcodes.begin(), stack_.size() - arg_count - 1});

                    break;
                }

                case Opcode::super_invoke: {
                    const auto method_name_constant_index = *frames_.back().opcode_iter++;
                    const auto method_name = boost::get<GC_ptr<std::string>>(
                        frames_.back().closure->function->chunk.constants.at(method_name_constant_index).variant
                    );
                    const auto arg_count = *frames_.back().opcode_iter++;

                    const auto superclass = boost::get<GC_ptr<Class>>(stack_.back().variant);
                    stack_.pop_back();

                    const auto method_iter = superclass->methods.find(method_name);
                    if (method_iter == superclass->methods.cend()) {
                        throw std::runtime_error{"Undefined property '" + *method_name + "'."};
                    }

                    const auto method_closure = boost::get<GC_ptr<Closure>>(method_iter->second.variant);
                    if (method_closure->function->arity != arg_count) {
                        const auto opcode_offset = frames_.back().opcode_iter - frames_.back().closure->function->chunk.opcodes.begin();
                        const auto debug_token = frames_.back().closure->function->chunk.tokens.at(opcode_offset - 1);
                        throw VM_runtime_error{debug_token, "Expected " + std::to_string(method_closure->function->arity) + " arguments but got " + std::to_string(arg_count) + "."};
                    }

                    frames_.push_back({method_closure, method_closure->function->chunk.opcodes.begin(), stack_.size() - arg_count - 1});

                    break;
                }

                case Opcode::closure: {
                    const auto function_constant_index = *frames_.back().opcode_iter++;
                    const auto function = boost::get<GC_ptr<Function>>(
                        frames_.back().closure->function->chunk.constants.at(function_constant_index).variant
                    );

                    const auto closure = gc_heap_.make(Closure{function});
                    stack_.push_back({closure});

                    for (auto n_upvalue = 0; n_upvalue != closure->function->upvalue_count; ++n_upvalue) {
                        const auto is_direct_capture = *frames_.back().opcode_iter++;
                        const auto enclosing_index = *frames_.back().opcode_iter++;

                        if (is_direct_capture) {
                            const auto local_stack_index = frames_.back().stack_begin_index + enclosing_index;
                            const auto open_upvalue_iter = std::find_if(open_upvalues_.crbegin(), open_upvalues_.crend(), [&] (const auto& upvalue) {
                                return upvalue->position == local_stack_index;
                            });
                            if (open_upvalue_iter == open_upvalues_.crend()) {
                                const auto open_upvalue = gc_heap_.make(Upvalue{stack_, local_stack_index});
                                closure->upvalues.push_back(open_upvalue);

                                // Insert sorted; maybe should use map?
                                const auto upvalue_insert_position = std::find_if(open_upvalues_.crbegin(), open_upvalues_.crend(), [&] (const auto& upvalue) {
                                    return upvalue->position < local_stack_index;
                                });
                                open_upvalues_.insert(upvalue_insert_position.base(), open_upvalue);
                            } else {
                                closure->upvalues.push_back(*open_upvalue_iter);
                            }
                        } else {
                            closure->upvalues.push_back(frames_.back().closure->upvalues.at(enclosing_index));
                        }
                    }

                    break;
                }

                case Opcode::close_upvalue: {
                    while (!open_upvalues_.empty() && open_upvalues_.back()->position >= stack_.size() - 1) {
                        open_upvalues_.back()->close();
                        open_upvalues_.pop_back();
                    }
                    stack_.pop_back();
                    break;
                }

                case Opcode::return_: {
                    while (!open_upvalues_.empty() && open_upvalues_.back()->position >= frames_.back().stack_begin_index) {
                        open_upvalues_.back()->close();
                        open_upvalues_.pop_back();
                    }

                    const auto return_value = stack_.back();
                    stack_.erase(stack_.begin() + frames_.back().stack_begin_index, stack_.end());
                    frames_.pop_back();

                    if (frames_.empty()) {
                        return;
                    }
                    stack_.push_back(return_value);

                    break;
                }

                case Opcode::class_: {
                    const auto class_name_constant_index = *frames_.back().opcode_iter++;
                    const auto class_name = boost::get<GC_ptr<std::string>>(
                        frames_.back().closure->function->chunk.constants.at(class_name_constant_index).variant
                    );
                    stack_.push_back({gc_heap_.make(Class{class_name})});
                    break;
                }

                case Opcode::inherit: {
                    const auto superclass = ([&] () {
                        try {
                            return boost::get<GC_ptr<Class>>((stack_.end() - 2)->variant);
                        } catch (const boost::bad_get&) {
                            const auto opcode_offset = frames_.back().opcode_iter - frames_.back().closure->function->chunk.opcodes.begin();
                            const auto debug_token = frames_.back().closure->function->chunk.tokens.at(opcode_offset - 1);
                            throw VM_runtime_error{debug_token, "Superclass must be a class."};
                        }
                    })();
                    const auto subclass = boost::get<GC_ptr<Class>>(stack_.back().variant);

                    subclass->methods.insert(superclass->methods.cbegin(), superclass->methods.cend());
                    stack_.pop_back();
                    break;
                }

                case Opcode::method: {
                    const auto method_name_constant_index = *frames_.back().opcode_iter++;
                    const auto method_name = boost::get<GC_ptr<std::string>>(
                        frames_.back().closure->function->chunk.constants.at(method_name_constant_index).variant
                    );

                    const auto klass = boost::get<GC_ptr<Class>>((stack_.end() - 2)->variant);
                    klass->methods[method_name] = stack_.back();
                    stack_.pop_back();

                    break;
                }
            }

            if (debug) {
                std::cout << stack_ << '\n';
            }
        }
    }
}}
