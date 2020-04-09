#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "chunk.hpp"
#include "value.hpp"

namespace motts { namespace lox {
    struct Call_frame {
        const Function& function;
        std::vector<std::uint8_t>::const_iterator ip;
        std::vector<Value>::iterator stack_begin;
    };

    class VM {
        public:
            void interpret(const std::string& source);

        private:
            void run();

            std::vector<Call_frame> frames_;
            std::vector<Value> stack_;
            std::unordered_map<std::string, Value> globals_;
    };

    struct VM_error : std::runtime_error {
        using std::runtime_error::runtime_error;
        VM_error(const std::string& what, const std::vector<Call_frame>&);
    };
}}
