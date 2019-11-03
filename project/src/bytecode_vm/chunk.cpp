#include "chunk.hpp"

namespace motts { namespace lox {
    void write_chunk(Chunk& chunk, Op_code opcode, int line) {
        chunk.code.push_back(static_cast<int>(opcode));
        chunk.lines.push_back(line);
    }

    void write_chunk(Chunk& chunk, int constant_offset, int line) {
        chunk.code.push_back(constant_offset);
        chunk.lines.push_back(line);
    }
}}
