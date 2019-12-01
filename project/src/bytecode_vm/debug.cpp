#include "debug.hpp"

#include <iomanip>
#include <iostream>

using std::cout;
using std::left;
using std::right;
using std::setfill;
using std::setw;
using std::string;
using std::uint16_t;

using namespace motts::lox;

// Not exported (internal linkage)
namespace {
    int simple_instrunction(const string& name) {
        cout << name << "\n";
        return 1;
    }

    int constant_instruction(const string& name, const Chunk& chunk, int code_offset) {
        const auto constant_offset = chunk.code.at(code_offset + 1);
        cout <<
            setw(16) << setfill(' ') << left << name << " " <<
            setw(4) << setfill('0') << right << static_cast<int>(constant_offset) << " '";
        print_value(chunk.constants.at(constant_offset));
        cout << "'\n";

        return 2;
    }

    int byte_instruction(const string& name, const Chunk& chunk, int code_offset) {
        const auto slot = chunk.code.at(code_offset + 1);
        cout <<
            setw(16) << setfill(' ') << left << name << " " <<
            setw(4) << setfill('0') << right << static_cast<int>(slot) <<
            "\n";
        return 2;
    }

    int jump_instruction(const string& name, int code_offset, int jump_length) {
        cout <<
            setw(16) << setfill(' ') << left << name << " " <<
            setw(4) << setfill('0') << right << static_cast<int>(code_offset) << " -> " <<
            setw(4) << setfill('0') << right << static_cast<int>(code_offset + jump_length) <<
            "\n";
        return 3;
    }

    int jump_instruction(const string& name, const Chunk& chunk, int code_offset) {
        // DANGER! Reinterpret cast: There will be two adjacent bytes
        // that are supposed to represent a single uint16 number
        const auto jump_length = reinterpret_cast<const uint16_t&>(chunk.code.at(code_offset + 1));
        return jump_instruction(name, code_offset, jump_length + 3);
    }

    int loop_instruction(const string& name, const Chunk& chunk, int code_offset) {
        // DANGER! Reinterpret cast: There will be two adjacent bytes
        // that are supposed to represent a single uint16 number
        const auto jump_length = reinterpret_cast<const uint16_t&>(chunk.code.at(code_offset + 1));
        return jump_instruction(name, code_offset, -jump_length);
    }

}

// Exported (external linkage)
namespace motts { namespace lox {
    int disassemble_instruction(const Chunk& chunk, int offset) {
        cout << setw(4) << setfill('0') << right << offset << " ";
        if (offset == 0 || chunk.lines.at(offset) != chunk.lines.at(offset - 1)) {
            cout << setw(4) << setfill(' ') << right << chunk.lines.at(offset) << " ";
        } else {
            cout << "   | ";
        }

        const auto instruction = static_cast<Op_code>(chunk.code.at(offset));
        switch (instruction) {
            case Op_code::constant:
                return constant_instruction("OP_CONSTANT", chunk, offset);
            case Op_code::nil:
                return simple_instrunction("OP_NIL");
            case Op_code::true_:
                return simple_instrunction("OP_TRUE");
            case Op_code::false_:
                return simple_instrunction("OP_FALSE");
            case Op_code::pop:
                return simple_instrunction("OP_POP");
            case Op_code::get_local:
                return byte_instruction("OP_GET_LOCAL", chunk, offset);
            case Op_code::set_local:
                return byte_instruction("OP_SET_LOCAL", chunk, offset);
            case Op_code::get_global:
                return constant_instruction("OP_GET_GLOBAL", chunk, offset);
            case Op_code::define_global:
                return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
            case Op_code::set_global:
                return constant_instruction("OP_SET_GLOBAL", chunk, offset);
            case Op_code::equal:
                return simple_instrunction("OP_EQUAL");
            case Op_code::greater:
                return simple_instrunction("OP_GREATER");
            case Op_code::less:
                return simple_instrunction("OP_LESS");
            case Op_code::add:
                return simple_instrunction("OP_ADD");
            case Op_code::subtract:
                return simple_instrunction("OP_SUBTRACT");
            case Op_code::multiply:
                return simple_instrunction("OP_MULTIPLY");
            case Op_code::divide:
                return simple_instrunction("OP_DIVIDE");
            case Op_code::not_:
                return simple_instrunction("OP_NOT");
            case Op_code::negate:
                return simple_instrunction("OP_NEGATE");
            case Op_code::print:
                return simple_instrunction("OP_PRINT");
            case Op_code::jump:
                return jump_instruction("OP_JUMP", chunk, offset);
            case Op_code::jump_if_false:
                return jump_instruction("OP_JUMP_IF_FALSE", chunk, offset);
            case Op_code::loop:
                return loop_instruction("OP_LOOP", chunk, offset);
            case Op_code::return_:
                return simple_instrunction("OP_RETURN");

            default:
                cout << "Unknown opcode " << static_cast<int>(instruction) << "\n";
                return 1;
        }
    }

    void disassemble_chunk(const Chunk& chunk, const string& name) {
        cout << "== " << name << " ==\n";

        for (auto iter = chunk.code.cbegin(); iter != chunk.code.cend(); ) {
            const auto instruction_length = disassemble_instruction(chunk, iter - chunk.code.cbegin());
            iter += instruction_length;
        }
    }
}}
