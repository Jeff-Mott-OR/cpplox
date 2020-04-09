#pragma once

#include "function_fwd.hpp"

#include <string>
#include <vector>

#include "chunk.hpp"
#include "value.hpp"

namespace motts { namespace lox {
    struct Function {
        int arity {0};
        Chunk chunk;
        std::string name;

        Function(std::string name_arg = "") :
            name {std::move(name_arg)}
        {}
    };

    struct Native_fn {
        Value (*fn)(std::vector<Value>::iterator args_begin, std::vector<Value>::iterator args_end);
    };
}}
