#include <stdio.h>

#include "debug.hpp"
#include "object.hpp"
#include "value.hpp"

#include <iomanip>
#include <iostream>

namespace {
    int constant_instruction(const char* name, const Chunk& chunk, int instruction_offset) {
        const auto constant_offset = chunk.code.at(instruction_offset + 1);

        std::cout <<
            std::setw(16) << std::setfill(' ') << std::left << name <<
            " " <<
            std::setw(4) << std::setfill(' ') << std::right << static_cast<int>(constant_offset) <<
            " '";
        std::cout << chunk.constants.at(constant_offset) << "'\n";

        return instruction_offset + 2;
    }

    int invoke_instruction(const char* name, const Chunk& chunk, int instruction_offset) {
        const auto constant_offset = chunk.code.at(instruction_offset + 1);
        const auto arg_count = chunk.code.at(instruction_offset + 2);

        std::cout <<
            std::setw(16) << std::setfill(' ') << std::left << name <<
            " (" << arg_count << " args)" <<
            std::setw(4) << std::setfill(' ') << std::right << constant_offset <<
            " '";
        std::cout << chunk.constants.at(constant_offset) << "'\n";

        return instruction_offset + 3;
    }

    int simple_instruction(const char* name, int instruction_offset) {
        std::cout << name << "\n";
        return instruction_offset + 1;
    }

    int byte_instruction(const char* name, const Chunk& chunk, int instruction_offset) {
        const auto slot = chunk.code.at(instruction_offset + 1);

        std::cout <<
            std::setw(16) << std::setfill(' ') << std::left << name <<
            " " <<
            std::setw(4) << std::setfill(' ') << std::right << static_cast<int>(slot) <<
            "\n";

        return instruction_offset + 2;
    }

    int jump_instruction(const char* name, int sign, const Chunk& chunk, int instruction_offset) {
        const auto jump = (static_cast<uint16_t>(chunk.code.at(instruction_offset + 1)) << 8) | chunk.code.at(instruction_offset + 2);

        std::cout <<
            std::setw(16) << std::setfill(' ') << std::left << name <<
            " " <<
            std::setw(4) << std::setfill(' ') << std::right << instruction_offset <<
            " -> " << (instruction_offset + 3 + sign * jump) << "\n";

        return instruction_offset + 3;
    }
}

void disassemble_chunk(const Chunk& chunk, const char* name) {
    std::cout << "== " << name << " ==\n";
    for (std::size_t instruction_offset = 0; instruction_offset < chunk.code.size(); ) {
        instruction_offset = disassemble_instruction(chunk, instruction_offset);
    }
}

int disassemble_instruction(const Chunk& chunk, int instruction_offset) {
    std::cout << std::setw(4) << std::setfill('0') << std::right << instruction_offset << " ";

    if (instruction_offset > 0 && chunk.lines.at(instruction_offset) == chunk.lines.at(instruction_offset - 1)) {
        std::cout << "   | ";
    } else {
        std::cout << std::setw(4) << std::setfill(' ') << std::right << chunk.lines.at(instruction_offset) << " ";
    }

    const auto instruction = static_cast<Op_code>(chunk.code.at(instruction_offset));
    switch (instruction) {
        case Op_code::constant_:
            return constant_instruction("CONSTANT", chunk, instruction_offset);
        case Op_code::nil_:
            return simple_instruction("NIL", instruction_offset);
        case Op_code::true_:
            return simple_instruction("TRUE", instruction_offset);
        case Op_code::false_:
            return simple_instruction("FALSE", instruction_offset);
        case Op_code::pop_:
            return simple_instruction("POP", instruction_offset);
        case Op_code::get_local_:
            return byte_instruction("GET_LOCAL", chunk, instruction_offset);
        case Op_code::set_local_:
            return byte_instruction("SET_LOCAL", chunk, instruction_offset);
        case Op_code::get_global_:
            return constant_instruction("GET_GLOBAL", chunk, instruction_offset);
        case Op_code::define_global_:
            return constant_instruction("DEFINE_GLOBAL", chunk, instruction_offset);
        case Op_code::set_global_:
            return constant_instruction("SET_GLOBAL", chunk, instruction_offset);
        case Op_code::get_upvalue_:
            return byte_instruction("GET_UPVALUE", chunk, instruction_offset);
        case Op_code::set_upvalue_:
            return byte_instruction("SET_UPVALUE", chunk, instruction_offset);
        case Op_code::get_property_:
            return constant_instruction("GET_PROPERTY", chunk, instruction_offset);
        case Op_code::set_property_:
            return constant_instruction("SET_PROPERTY", chunk, instruction_offset);
        case Op_code::get_super_:
            return constant_instruction("GET_SUPER", chunk, instruction_offset);
        case Op_code::equal_:
            return simple_instruction("EQUAL", instruction_offset);
        case Op_code::greater_:
            return simple_instruction("GREATER", instruction_offset);
        case Op_code::less_:
            return simple_instruction("LESS", instruction_offset);
        case Op_code::add_:
            return simple_instruction("ADD", instruction_offset);
        case Op_code::subtract_:
            return simple_instruction("SUBTRACT", instruction_offset);
        case Op_code::multiply_:
            return simple_instruction("MULTIPLY", instruction_offset);
        case Op_code::divide_:
            return simple_instruction("DIVIDE", instruction_offset);
        case Op_code::not_:
            return simple_instruction("NOT", instruction_offset);
        case Op_code::negate_:
            return simple_instruction("NEGATE", instruction_offset);
        case Op_code::print_:
            return simple_instruction("PRINT", instruction_offset);
        case Op_code::jump_:
            return jump_instruction("JUMP", 1, chunk, instruction_offset);
        case Op_code::jump_if_false_:
            return jump_instruction("JUMP_IF_FALSE", 1, chunk, instruction_offset);
        case Op_code::loop_:
            return jump_instruction("LOOP", -1, chunk, instruction_offset);
        case Op_code::call_:
            return byte_instruction("CALL", chunk, instruction_offset);
        case Op_code::invoke_:
            return invoke_instruction("INVOKE", chunk, instruction_offset);
        case Op_code::super_invoke_:
            return invoke_instruction("SUPER_INVOKE", chunk, instruction_offset);
        case Op_code::closure_: {
            const auto function_offset = chunk.code.at(instruction_offset + 1);

            std::cout <<
                std::setw(16) << std::setfill(' ') << std::left << "CLOSURE" <<
                " " <<
                std::setw(4) << std::setfill(' ') << std::right << static_cast<int>(function_offset) <<
                " ";
            std::cout << chunk.constants.at(function_offset) << "\n";

            ObjFunction* function = boost::get<ObjFunction*>((chunk.constants.at(function_offset).variant));
            for (int n_upvalue = 0; n_upvalue < function->upvalueCount; n_upvalue++) {
                const auto is_local = chunk.code.at(instruction_offset + 2 + n_upvalue * 2);
                const auto index = chunk.code.at(instruction_offset + 2 + n_upvalue * 2 + 1);

                std::cout <<
                    std::setw(4) << std::setfill('0') << std::right << static_cast<int>(instruction_offset + n_upvalue * 2) <<
                    "      |                     " << (is_local ? "local" : "upvalue") << " " << static_cast<int>(index) << "\n";
            }

            return instruction_offset + 2 + function->upvalueCount * 2;
        }
        case Op_code::close_upvalue_:
            return simple_instruction("CLOSE_UPVALUE", instruction_offset);
        case Op_code::return_:
            return simple_instruction("RETURN", instruction_offset);
        case Op_code::class_:
            return constant_instruction("CLASS", chunk, instruction_offset);
        case Op_code::inherit_:
            return simple_instruction("INHERIT", instruction_offset);
        case Op_code::method_:
            return constant_instruction("METHOD", chunk, instruction_offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return instruction_offset + 1;
    }
}
