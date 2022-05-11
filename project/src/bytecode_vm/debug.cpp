#include "debug.hpp"

#include <iomanip>

// Not exported (internal linkage)
namespace {
    // Allow the internal linkage section to access names
    using namespace motts::lox;

    auto print_simple_instruction(std::ostream& os, std::vector<std::uint8_t>::const_iterator opcode_iter) {
        os << static_cast<Opcode>(*opcode_iter) << '\n';
        return opcode_iter + 1;
    }

    auto print_constant_instruction(std::ostream& os, std::vector<std::uint8_t>::const_iterator opcode_iter, const std::vector<Value>& constants) {
        const auto constant_index = *(opcode_iter + 1);
        os << std::setw(16) << std::setfill(' ') << std::left << static_cast<Opcode>(*opcode_iter) << ' ' <<
            std::setw(4) << std::setfill(' ') << std::right << static_cast<int>(constant_index) <<
            " -> " << constants.at(constant_index) << '\n';
        return opcode_iter + 2;
    }

    auto print_byte_instruction(std::ostream& os, std::vector<std::uint8_t>::const_iterator opcode_iter) {
        os << std::setw(16) << std::setfill(' ') << std::left << static_cast<Opcode>(*opcode_iter) << ' ' <<
            std::setw(4) << std::setfill(' ') << std::right << static_cast<int>(*(opcode_iter + 1)) << '\n';
        return opcode_iter + 2;
    }

    auto print_jump_instruction(std::ostream& os, std::vector<std::uint8_t>::const_iterator opcode_iter, const Chunk& chunk, int sign) {
        const auto jump_distance = (*(opcode_iter + 1) << 8) | *(opcode_iter + 2);
        const auto jump_origin_offset = opcode_iter - chunk.opcodes.cbegin();
        const auto jump_dest_offset = opcode_iter + 3 + jump_distance * sign - chunk.opcodes.cbegin();
        os << std::setw(16) << std::setfill(' ') << std::left << static_cast<Opcode>(*opcode_iter) << ' ' <<
            std::setw(4) << std::setfill(' ') << std::right << jump_origin_offset << " -> " << jump_dest_offset << '\n';
        return opcode_iter + 3;
    }

    auto print_invoke_instruction(std::ostream& os, std::vector<std::uint8_t>::const_iterator opcode_iter, const std::vector<Value>& constants) {
        const auto property_name_constant_index = *(opcode_iter + 1);
        const auto arg_count = *(opcode_iter + 2);
        os << std::setw(16) << std::setfill(' ') << std::left << static_cast<Opcode>(*opcode_iter) <<
            std::setw(4) << std::setfill(' ') << std::right << static_cast<int>(property_name_constant_index) <<
            " -> " << constants.at(property_name_constant_index) << " (" << static_cast<int>(arg_count) << " args)\n";
        return opcode_iter + 3;
    }

    struct Disassemble_function_value_visitor : boost::static_visitor<void> {
        std::ostream& os;

        Disassemble_function_value_visitor(std::ostream& os_arg) :
            os {os_arg}
        {}

        // Functions are disassembled
        auto operator()(const GC_ptr<Function>& fn) const {
            disassemble(os, fn);
        }

        // All other value types do nothing
        template<typename T>
            auto operator()(const T&) const {
            }
    };
}

namespace motts { namespace lox {
    void disassemble(std::ostream& os, const GC_ptr<Function>& fn) {
        os << "== " << *fn->name << " ==\n";
        for (auto opcode_iter = fn->chunk.opcodes.cbegin(); opcode_iter != fn->chunk.opcodes.cend(); ) {
             const auto instruction_end_iter = disassemble_instruction(os, opcode_iter, fn->chunk);
             opcode_iter = instruction_end_iter;
        }

        for (const auto& constant : fn->chunk.constants) {
            boost::apply_visitor(Disassemble_function_value_visitor{os}, constant.variant);
        }
    }

    std::vector<std::uint8_t>::const_iterator disassemble_instruction(std::ostream& os, std::vector<std::uint8_t>::const_iterator opcode_iter, const Chunk& chunk) {
        const auto instruction_index = opcode_iter - chunk.opcodes.cbegin();
        os << std::setw(4) << std::setfill('0') << std::right << instruction_index << ' ';

        if (instruction_index == 0 || chunk.tokens.at(instruction_index).line != chunk.tokens.at(instruction_index - 1).line) {
            os << std::setw(4) << std::setfill(' ') << std::right << chunk.tokens.at(instruction_index).line << ' ';
        } else {
            os << "   | ";
        }

        switch (static_cast<Opcode>(*opcode_iter)) {
            default:
                throw std::runtime_error{"Unknown opcode"};

            case Opcode::nil:           return print_simple_instruction(os, opcode_iter);
            case Opcode::true_:         return print_simple_instruction(os, opcode_iter);
            case Opcode::false_:        return print_simple_instruction(os, opcode_iter);
            case Opcode::pop:           return print_simple_instruction(os, opcode_iter);
            case Opcode::equal:         return print_simple_instruction(os, opcode_iter);
            case Opcode::greater:       return print_simple_instruction(os, opcode_iter);
            case Opcode::less:          return print_simple_instruction(os, opcode_iter);
            case Opcode::add:           return print_simple_instruction(os, opcode_iter);
            case Opcode::subtract:      return print_simple_instruction(os, opcode_iter);
            case Opcode::multiply:      return print_simple_instruction(os, opcode_iter);
            case Opcode::divide:        return print_simple_instruction(os, opcode_iter);
            case Opcode::not_:          return print_simple_instruction(os, opcode_iter);
            case Opcode::negate:        return print_simple_instruction(os, opcode_iter);
            case Opcode::print:         return print_simple_instruction(os, opcode_iter);
            case Opcode::close_upvalue: return print_simple_instruction(os, opcode_iter);
            case Opcode::return_:       return print_simple_instruction(os, opcode_iter);
            case Opcode::inherit:       return print_simple_instruction(os, opcode_iter);

            case Opcode::constant:      return print_constant_instruction(os, opcode_iter, chunk.constants);
            case Opcode::get_global:    return print_constant_instruction(os, opcode_iter, chunk.constants);
            case Opcode::define_global: return print_constant_instruction(os, opcode_iter, chunk.constants);
            case Opcode::set_global:    return print_constant_instruction(os, opcode_iter, chunk.constants);
            case Opcode::get_property:  return print_constant_instruction(os, opcode_iter, chunk.constants);
            case Opcode::set_property:  return print_constant_instruction(os, opcode_iter, chunk.constants);
            case Opcode::get_super:     return print_constant_instruction(os, opcode_iter, chunk.constants);
            case Opcode::class_:        return print_constant_instruction(os, opcode_iter, chunk.constants);
            case Opcode::method:        return print_constant_instruction(os, opcode_iter, chunk.constants);

            case Opcode::get_local:     return print_byte_instruction(os, opcode_iter);
            case Opcode::set_local:     return print_byte_instruction(os, opcode_iter);
            case Opcode::get_upvalue:   return print_byte_instruction(os, opcode_iter);
            case Opcode::set_upvalue:   return print_byte_instruction(os, opcode_iter);
            case Opcode::call:          return print_byte_instruction(os, opcode_iter);

            case Opcode::jump:          return print_jump_instruction(os, opcode_iter, chunk,  1);
            case Opcode::jump_if_false: return print_jump_instruction(os, opcode_iter, chunk,  1);
            case Opcode::loop:          return print_jump_instruction(os, opcode_iter, chunk, -1);

            case Opcode::invoke:        return print_invoke_instruction(os, opcode_iter, chunk.constants);
            case Opcode::super_invoke:  return print_invoke_instruction(os, opcode_iter, chunk.constants);

            case Opcode::closure: {
                const auto function_constant_index = *(opcode_iter + 1);
                os << std::setw(16) << std::setfill(' ') << std::left << static_cast<Opcode>(*opcode_iter) << ' ' <<
                    std::setw(4) << std::setfill(' ') << std::right << static_cast<int>(function_constant_index) <<
                    " -> " << chunk.constants.at(function_constant_index) << '\n';

                const auto function = boost::get<GC_ptr<Function>>(chunk.constants.at(function_constant_index).variant);
                for (auto n_upvalue = 0; n_upvalue != function->upvalue_count; ++n_upvalue) {
                    const auto is_direct_capture = *(opcode_iter + 2 + n_upvalue * 2);
                    const auto enclosing_index = *(opcode_iter + 2 + n_upvalue * 2 + 1);
                    os << std::setw(4) << std::setfill('0') << std::right << (instruction_index + 2 + n_upvalue * 2) <<
                        "      |                   " << (is_direct_capture ? "direct" : "indirect") <<
                        " -> " << static_cast<int>(enclosing_index) << '\n';
                }

                return opcode_iter + 2 + function->upvalue_count * 2;
            }
        }
    }

    std::ostream& operator<<(std::ostream& os, const std::vector<Value>& stack) {
        os << "+----\n";
        for (auto value_iter = stack.crbegin(); value_iter != stack.crend(); ++value_iter) {
            os << "| " << *value_iter << '\n';
        }
        os << "+----\n";
        return os;
    }
}}
