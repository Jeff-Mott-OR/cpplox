#pragma once

#include "chunk.hpp"

namespace motts { namespace lox {
    class VM {
        public:
            void interpret(const Chunk&);

        private:
            void run();

            const Chunk* chunk_;
            std::vector<int>::const_iterator ip_;
            std::vector<Value> stack_;
    };
}}
