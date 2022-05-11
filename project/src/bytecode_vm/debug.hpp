#pragma once

#include <ostream>
#include <vector>

#include "chunk.hpp"
#include "memory.hpp"
#include "object.hpp"
#include "value.hpp"

namespace motts { namespace lox {
    void disassemble(std::ostream&, const GC_ptr<Function>&);

    std::vector<std::uint8_t>::const_iterator disassemble_instruction(std::ostream& os, std::vector<std::uint8_t>::const_iterator opcode_iter, const Chunk& chunk);

    std::ostream& operator<<(std::ostream&, const std::vector<Value>& stack);
}}
