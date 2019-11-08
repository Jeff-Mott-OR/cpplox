#pragma once

#include <stdexcept>
#include <string>

#include "chunk.hpp"

namespace motts { namespace lox {
    class VM {
        public:
            void interpret(const std::string& source);

        private:
            void run();

            const Chunk* chunk_;
            std::vector<unsigned char>::const_iterator ip_;
            std::vector<Value> stack_;
    };

    struct VM_error : std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}}
