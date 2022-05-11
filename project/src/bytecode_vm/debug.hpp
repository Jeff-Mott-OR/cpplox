#pragma once

#include "chunk.hpp"

void disassemble_chunk(const Chunk& chunk, const char* name);
int disassemble_instruction(const Chunk& chunk, int offset);
