#include "vm.hpp"

#include <cstddef>
#include <iostream>
#include <utility>

#include "compiler.hpp"
#include "debug.hpp"

using std::cout;
using std::move;
using std::nullptr_t;
using std::string;

using boost::apply_visitor;
using boost::get;
using boost::static_visitor;

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
}

namespace motts { namespace lox {
    void VM::interpret(const string& source) {
      const auto chunk = compile(source);

      chunk_ = &chunk;
      ip_ = chunk.code.cbegin();

      run();
    }

    void VM::run() {
        for (;;) {
            cout << "          ";
            for (const auto value : stack_) {
                cout << "[ ";
                print_value(value);
                cout << " ]";
            }
            cout << "\n";
            disassemble_instruction(*chunk_, ip_ - chunk_->code.cbegin());

            const auto instruction = static_cast<Op_code>(*ip_++);
            switch (instruction) {
                case Op_code::constant: {
                    const auto constant_offset = *ip_++;
                    const auto constant = chunk_->constants.at(constant_offset);
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
                    const auto right_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    const auto left_value = get<double>(stack_.back().variant);
                    stack_.pop_back();

                    stack_.push_back(Value{left_value + right_value});

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

                case Op_code::return_: {
                    print_value(stack_.back());
                    cout << "\n";
                    stack_.pop_back();

                    return;
                }
            }
        }
    }
}}
