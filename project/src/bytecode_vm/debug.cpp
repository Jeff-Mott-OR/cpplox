#include "debug.hpp"

#include <iomanip>
#include <iostream>

using std::cout;
using std::left;
using std::right;
using std::setfill;
using std::setw;
using std::string;

using namespace motts::lox;

// Not exported (internal linkage)
namespace {
    int simple_instrunction(const string& name, int offset) {
        cout << name << "\n";
        return offset + 1;
    }

    int constant_instruction(const string& name, const Chunk& chunk, int code_offset) {
        const auto constant_offset = chunk.code[code_offset + 1];
        cout << setw(16) << setfill(' ') << left << name << " " <<
            setw(4) << setfill(' ') << right << constant_offset << " '";
        print_value(chunk.constants.at(constant_offset));
        cout << "'\n";

        return code_offset + 2;
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
                return simple_instrunction("OP_NIL", offset);
            case Op_code::true_:
                return simple_instrunction("OP_TRUE", offset);
            case Op_code::false_:
                return simple_instrunction("OP_FALSE", offset);

            case Op_code::equal:
                return simple_instrunction("OP_EQUAL", offset);
            case Op_code::greater:
                return simple_instrunction("OP_GREATER", offset);
            case Op_code::less:
                return simple_instrunction("OP_LESS", offset);

            case Op_code::add:
                return simple_instrunction("OP_ADD", offset);
            case Op_code::subtract:
                return simple_instrunction("OP_SUBTRACT", offset);
            case Op_code::multiply:
                return simple_instrunction("OP_MULTIPLY", offset);
            case Op_code::divide:
                return simple_instrunction("OP_DIVIDE", offset);

            case Op_code::not_:
                return simple_instrunction("OP_NOT", offset);
            case Op_code::negate:
                return simple_instrunction("OP_NEGATE", offset);

            case Op_code::return_:
                return simple_instrunction("OP_RETURN", offset);

            default:
                cout << "Unknown opcode " << static_cast<int>(instruction) << "\n";
                return offset + 1;
        }
    }

    void disassemble_chunk(const Chunk& chunk, const string& name) {
        cout << "== " << name << " ==\n";

        for (auto offset = 0; offset < chunk.code.size(); ) {
            const auto next_offset = disassemble_instruction(chunk, offset);
            offset = next_offset;
        }
    }
}}
