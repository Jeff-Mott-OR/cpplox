#pragma once

#include <string>
#include "chunk.hpp"

namespace motts { namespace lox {
    void disassemble_chunk(const Chunk& chunk, const std::string& name);
}}
