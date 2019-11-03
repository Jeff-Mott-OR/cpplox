#include "chunk.hpp"
#include "debug.hpp"

namespace loxns = motts::lox;

int main(int argc, const char* argv[]) {
    loxns::Chunk chunk;

    chunk.constants.push_back(1.2);
    chunk.code.push_back(static_cast<int>(loxns::Op_code::constant));
    chunk.lines.push_back(123);
    chunk.code.push_back(chunk.constants.size() - 1);
    chunk.lines.push_back(123);

    chunk.code.push_back(static_cast<int>(loxns::Op_code::return_));
    chunk.lines.push_back(123);

    loxns::disassemble_chunk(chunk, "test chunk");
}
