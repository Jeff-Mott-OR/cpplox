#include "vm.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <utility>

#include <gsl/gsl_util>

#include "compiler.hpp"
#include "debug.hpp"

using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;
using std::cout;
using std::move;
using std::nullptr_t;
using std::string;
using std::to_string;
using std::uint8_t;
using std::uint16_t;
using std::vector;

using boost::apply_visitor;
using boost::bad_get;
using boost::get;
using boost::static_visitor;
using gsl::narrow;

using namespace motts::lox;

namespace {
    // Only false and nil are falsey, everything else is truthy
    struct Is_truthy_visitor : static_visitor<bool> {
        auto operator()(bool value) const {
            return value;
        }

        auto operator()(nullptr_t) const {
            return false;
        }

        template<typename T>
            auto operator()(const T&) const {
                return true;
            }
    };

    struct Is_equal_visitor : static_visitor<bool> {
        // Different types automatically compare false
        template<typename T, typename U>
            auto operator()(const T&, const U&) const {
                return false;
            }

        // Same types use normal equality test
        template<typename T>
            auto operator()(const T& lhs, const T& rhs) const {
                return lhs == rhs;
            }
    };

    struct Plus_visitor : static_visitor<Value> {
        auto operator()(const string& lhs, const string& rhs) const {
            return Value{lhs + rhs};
        }

        auto operator()(double lhs, double rhs) const {
            return Value{lhs + rhs};
        }

        // All other type combinations can't be '+'-ed together
        template<typename T, typename U>
            Value operator()(const T&, const U&) const {
                throw VM_error{"Operands must be two numbers or two strings."};
            }
    };

    struct Call_visitor : static_visitor<void> {
        // "Capture" variables, like a lamba but the long-winded way
        uint8_t& arg_count;
        vector<Call_frame>& frames_;
        vector<Value>& stack_;
        explicit Call_visitor(
            uint8_t& arg_count_capture,
            vector<Call_frame>& frames_capture,
            vector<Value>& stack_capture
        ) :
            arg_count {arg_count_capture},
            frames_ {frames_capture},
            stack_ {stack_capture}
        {}

        auto operator()(const Function* callee) const {
            if (arg_count != callee->arity) {
                throw VM_error{
                    "Expected " + to_string(callee->arity) +
                    " arguments but got " + to_string(arg_count) + ".",
                    frames_
                };
            }

            frames_.push_back(Call_frame{
                *callee,
                callee->chunk.code.cbegin(),
                stack_.end() - 1 - arg_count
            });
        }

        auto operator()(const Native_fn* callee) const {
            auto return_value = callee->fn(stack_.end() - arg_count, stack_.end());
            stack_.erase(stack_.end() - 1 - arg_count, stack_.end());
            stack_.push_back(return_value);
        }

        template<typename T>
            void operator()(const T&) const {
                throw VM_error{"Can only call functions and classes.", frames_};
            }
    };

    auto clock_native(vector<Value>::iterator /*args_begin*/, vector<Value>::iterator /*args_end*/) {
        return Value{narrow<double>(duration_cast<seconds>(system_clock::now().time_since_epoch()).count())};
    }

    string stack_trace(const vector<Call_frame>& frames) {
        string trace;

        for (auto frame_iter = frames.crbegin(); frame_iter != frames.crend(); ++frame_iter) {
            const auto instruction_offset = frame_iter->ip - 1 - frame_iter->function.chunk.code.cbegin();
            const auto line = frame_iter->function.chunk.lines.at(instruction_offset);
            trace += "[Line " + to_string(line) + "] in " + (
                frame_iter->function.name.empty() ?
                    "script" :
                    frame_iter->function.name + "()"
            ) + "\n";
        }

        return trace;
    }
}

namespace motts { namespace lox {
    void VM::interpret(const string& source) {
        // TODO: Allow dynamically-sized stack but without invalidating iterators.
        // Maybe deque? Or offsets instead of iterators? For now just pre-allocate.
        stack_.reserve(256);

        auto entry_point_function = compile(source);
        stack_.push_back(Value{&entry_point_function});

        frames_.push_back(Call_frame{
            entry_point_function,
            entry_point_function.chunk.code.cbegin(),
            stack_.begin()
        });

        globals_["clock"] = Value{new Native_fn{clock_native}};

        run();
    }

    void VM::run() {
        for (;;) {
            #ifndef NDEBUG
                cout << "          ";
                for (const auto value : stack_) {
                    cout << "[ ";
                    print_value(value);
                    cout << " ]";
                }
                cout << "\n";
                disassemble_instruction(frames_.back().function.chunk, frames_.back().ip - frames_.back().function.chunk.code.cbegin());
            #endif

            const auto instruction = static_cast<Op_code>(*frames_.back().ip++);
            switch (instruction) {
                case Op_code::constant: {
                    const auto constant_offset = *frames_.back().ip++;
                    const auto constant = frames_.back().function.chunk.constants.at(constant_offset);
                    stack_.push_back(constant);

                    break;
                }

                case Op_code::nil: {
                    stack_.push_back(Value{nullptr});
                    break;
                }

                case Op_code::true_: {
                    stack_.push_back(Value{true});
                    break;
                }

                case Op_code::false_: {
                    stack_.push_back(Value{false});
                    break;
                }

                case Op_code::pop: {
                    stack_.pop_back();
                    break;
                }

                case Op_code::get_local: {
                    const auto local_offset = *frames_.back().ip++;
                    stack_.push_back(*(frames_.back().stack_begin + local_offset));
                    break;
                }

                case Op_code::set_local: {
                    const auto local_offset = *frames_.back().ip++;
                    *(frames_.back().stack_begin + local_offset) = stack_.back();
                    break;
                }

                case Op_code::get_global: {
                    const auto name_offset = *frames_.back().ip++;
                    const auto name = get<string>(frames_.back().function.chunk.constants.at(name_offset).variant);
                    const auto found_value = globals_.find(name);
                    if (found_value == globals_.end()) {
                        throw VM_error{"Undefined variable '" + name + "'", frames_};
                    }
                    stack_.push_back(found_value->second);

                    break;
                }

                case Op_code::set_global: {
                    const auto name_offset = *frames_.back().ip++;
                    const auto name = get<string>(frames_.back().function.chunk.constants.at(name_offset).variant);
                    const auto found_value = globals_.find(name);
                    if (found_value == globals_.end()) {
                        throw VM_error{"Undefined variable '" + name + "'", frames_};
                    }
                    found_value->second = stack_.back();

                    break;
                }

                case Op_code::define_global: {
                    const auto name_offset = *frames_.back().ip++;
                    const auto name = get<string>(frames_.back().function.chunk.constants.at(name_offset).variant);
                    globals_[name] = stack_.back();
                    stack_.pop_back();

                    break;
                }

                case Op_code::equal: {
                    const auto right_value = move(stack_.back());
                    stack_.pop_back();

                    const auto left_value = move(stack_.back());
                    stack_.pop_back();

                    stack_.push_back(Value{
                        apply_visitor(Is_equal_visitor{}, left_value.variant, right_value.variant)
                    });

                    break;
                }

                case Op_code::greater: {
                    const auto right_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    const auto left_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    stack_.push_back(Value{left_value > right_value});

                    break;
                }

                case Op_code::less: {
                    const auto right_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    const auto left_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    stack_.push_back(Value{left_value < right_value});

                    break;
                }

                case Op_code::add: {
                    const auto right_value = move(stack_.back());
                    stack_.pop_back();

                    const auto left_value = move(stack_.back());
                    stack_.pop_back();

                    stack_.push_back(apply_visitor(Plus_visitor{}, left_value.variant, right_value.variant));

                    break;
                }

                case Op_code::subtract: {
                    const auto right_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    const auto left_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    stack_.push_back(Value{left_value - right_value});

                    break;
                }

                case Op_code::multiply: {
                    const auto right_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    const auto left_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    stack_.push_back(Value{left_value * right_value});

                    break;
                }

                case Op_code::divide: {
                    const auto right_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    const auto left_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    stack_.push_back(Value{left_value / right_value});

                    break;
                }

                case Op_code::not_: {
                    const auto value = apply_visitor(Is_truthy_visitor{}, stack_.back().variant);
                    stack_.pop_back();
                    stack_.push_back(Value{!value});

                    break;
                }

                case Op_code::negate: {
                    // TODO Convert error: "Operand must be a number."
                    const auto value = get<double>(stack_.back().variant);
                    stack_.pop_back();
                    stack_.push_back(Value{-value});

                    break;
                }

                case Op_code::print: {
                    print_value(stack_.back());
                    cout << "\n";
                    stack_.pop_back();

                    break;
                }

                case Op_code::jump: {
                    // DANGER! Reinterpret cast: The two bytes following a jump_if_false instruction
                    // are supposed to represent a single uint16 number
                    const auto jump_length = reinterpret_cast<const uint16_t&>(*(frames_.back().ip));
                    frames_.back().ip += 2;

                    frames_.back().ip += jump_length;

                    break;
                }

                case Op_code::jump_if_false: {
                    // DANGER! Reinterpret cast: The two bytes following a jump_if_false instruction
                    // are supposed to represent a single uint16 number
                    const auto jump_length = reinterpret_cast<const uint16_t&>(*(frames_.back().ip));
                    frames_.back().ip += 2;

                    if (!apply_visitor(Is_truthy_visitor{}, stack_.back().variant)) {
                        frames_.back().ip += jump_length;
                    }

                    break;
                }

                case Op_code::loop: {
                    // DANGER! Reinterpret cast: The two bytes following a loop instruction
                    // are supposed to represent a single uint16 number
                    const auto jump_length = reinterpret_cast<const uint16_t&>(*(frames_.back().ip));
                    frames_.back().ip -= 1;

                    frames_.back().ip -= jump_length;

                    break;
                }

                case Op_code::call: {
                    auto arg_count = *frames_.back().ip++;
                    Call_visitor call_visitor {arg_count, frames_, stack_};
                    apply_visitor(call_visitor, (stack_.end() - 1 - arg_count)->variant);

                    break;
                }

                case Op_code::return_: {
                    const auto return_value = stack_.back();

                    stack_.erase(frames_.back().stack_begin, stack_.end());
                    frames_.pop_back();

                    if (frames_.empty()) {
                        return;
                    }

                    stack_.push_back(return_value);

                    break;
                }
            }
        }
    }

    VM_error::VM_error(const string& what, const vector<Call_frame>& frames) :
        VM_error{what + "\n" + stack_trace(frames)}
    {}
}}
