#ifndef clox_compiler_h
#define clox_compiler_h

#include "object.hpp"
#include "vm.hpp"

ObjFunction* compile(const char* source);

#endif
