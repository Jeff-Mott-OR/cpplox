#include "chunk.hpp"
#include <gsl/gsl_util>

using std::size_t;
using std::uint8_t;
using std::vector;

using gsl::narrow;

namespace motts { namespace lox {
    void Chunk::bytecode_push_back(Op_code opcode, int line) {
        bytecode_push_back(narrow<uint8_t>(static_cast<int>(opcode)), line);
    }

    void Chunk::bytecode_push_back(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }

    void Chunk::bytecode_push_back(int byte, int line) {
        bytecode_push_back(narrow<uint8_t>(byte), line);
    }

    void Chunk::bytecode_push_back(size_t byte, int line) {
        bytecode_push_back(narrow<uint8_t>(byte), line);
    }

    void Chunk::bytecode_push_back(ptrdiff_t byte, int line) {
        bytecode_push_back(narrow<uint8_t>(byte), line);
    }

    vector<Value>::size_type Chunk::constants_push_back(Value value) {
        const auto offset = constants.size();
        constants.push_back(value);
        return offset;
    }
}}
