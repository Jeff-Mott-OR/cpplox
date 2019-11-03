#include "chunk.hpp"
#include <gsl/gsl_util>

using std::vector;
using gsl::narrow;

namespace motts { namespace lox {
    void Chunk::bytecode_push_back(Op_code opcode, int line) {
        bytecode_push_back(narrow<unsigned char>(static_cast<int>(opcode)), line);
    }

    void Chunk::bytecode_push_back(unsigned char byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }

    vector<Value>::size_type Chunk::constants_push_back(Value value) {
        const auto offset = constants.size();
        constants.push_back(value);
        return offset;
    }
}}
