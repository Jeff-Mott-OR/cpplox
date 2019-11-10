#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>

#include "chunk.hpp"
#include "value.hpp"

namespace motts { namespace lox {
    class VM {
        public:
            void interpret(const std::string& source);

        private:
            void run();

            const Chunk* chunk_;
            std::vector<unsigned char>::const_iterator ip_;
            std::vector<Value> stack_;
            std::unordered_map<std::string, Value> globals_;
    };

    struct VM_error : std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}}
