#pragma once

#include <iostream>
#include <ostream>
#include <unordered_map>
#include <vector>

#include "compiler.hpp"

namespace motts { namespace lox {
    void run(const Chunk&, std::ostream& = std::cout, bool debug = false);

    class VM {
        std::vector<Dynamic_type_value> stack_;
        std::unordered_map<std::string, Dynamic_type_value> globals_;

        public:
            void run(const Chunk&, std::ostream& = std::cout, bool debug = false);
    };
}}
