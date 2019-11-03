#pragma once

#include <string>
#include "chunk.hpp"

namespace motts { namespace lox {
    int disassemble_instruction(const Chunk&, int offset);
    void disassemble_chunk(const Chunk&, const std::string& name);
}}
