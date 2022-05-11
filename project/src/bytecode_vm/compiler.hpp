#pragma once

#include "object.hpp"
#include "vm.hpp"

ObjFunction* compile(const std::string& source, Deferred_heap&, Interned_strings&);
