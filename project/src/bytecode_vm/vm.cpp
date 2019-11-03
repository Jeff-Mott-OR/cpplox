#include "vm.hpp"

#include <iostream>

#include "compiler.hpp"
#include "debug.hpp"

using std::cout;
using std::string;

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

                case Op_code::add: {
                    const auto right_value = stack_.back();
                    stack_.pop_back();

                    const auto left_value = stack_.back();
                    stack_.pop_back();

                    stack_.push_back(left_value + right_value);

                    break;
                }

                case Op_code::subtract: {
                    const auto right_value = stack_.back();
                    stack_.pop_back();

                    const auto left_value = stack_.back();
                    stack_.pop_back();

                    stack_.push_back(left_value - right_value);

                    break;
                }

                case Op_code::multiply: {
                    const auto right_value = stack_.back();
                    stack_.pop_back();

                    const auto left_value = stack_.back();
                    stack_.pop_back();

                    stack_.push_back(left_value * right_value);

                    break;
                }

                case Op_code::divide: {
                    const auto right_value = stack_.back();
                    stack_.pop_back();

                    const auto left_value = stack_.back();
                    stack_.pop_back();

                    stack_.push_back(left_value / right_value);

                    break;
                }

                case Op_code::negate: {
                    const auto value = stack_.back();
                    stack_.pop_back();
                    stack_.push_back(-value);

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
