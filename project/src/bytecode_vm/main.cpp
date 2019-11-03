#include "chunk.hpp"
#include "debug.hpp"
#include "vm.hpp"

namespace loxns = motts::lox;

int main(int argc, const char* argv[]) {
    loxns::VM vm;

    loxns::Chunk chunk;

    chunk.constants.push_back(1.2);
    loxns::write_chunk(chunk, loxns::Op_code::constant, 123);
    loxns::write_chunk(chunk, chunk.constants.size() - 1, 123);

    chunk.constants.push_back(3.4);
    loxns::write_chunk(chunk, loxns::Op_code::constant, 123);
    loxns::write_chunk(chunk, chunk.constants.size() - 1, 123);

    loxns::write_chunk(chunk, loxns::Op_code::add, 123);

    chunk.constants.push_back(5.6);
    loxns::write_chunk(chunk, loxns::Op_code::constant, 123);
    loxns::write_chunk(chunk, chunk.constants.size() - 1, 123);

    loxns::write_chunk(chunk, loxns::Op_code::divide, 123);
    loxns::write_chunk(chunk, loxns::Op_code::negate, 123);
    loxns::write_chunk(chunk, loxns::Op_code::return_, 123);

    loxns::disassemble_chunk(chunk, "test chunk");

    vm.interpret(chunk);
}
