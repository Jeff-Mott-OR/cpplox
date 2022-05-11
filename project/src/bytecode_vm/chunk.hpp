#pragma once

#include <vector>
#include "value.hpp"

enum class Op_code {
    constant_,
    nil_,
    true_,
    false_,
    pop_,
    get_local_,
    set_local_,
    get_global_,
    define_global_,
    set_global_,
    get_upvalue_,
    set_upvalue_,
    get_property_,
    set_property_,
    get_super_,
    equal_,
    greater_,
    less_,
    add_,
    subtract_,
    multiply_,
    divide_,
    not_,
    negate_,
    print_,
    jump_,
    jump_if_false_,
    loop_,
    call_,
    invoke_,
    super_invoke_,
    closure_,
    close_upvalue_,
    return_,
    class_,
    inherit_,
    method_
};

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<int> lines;
    std::vector<Value> constants;
};
