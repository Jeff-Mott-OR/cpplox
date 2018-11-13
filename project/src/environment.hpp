#pragma once

#include <string>
#include <utility>
#include <vector>

#include "literal.hpp"

namespace motts { namespace lox {
    class Environment {
        public:
            explicit Environment();
            explicit Environment(Environment* enclosed);
            std::vector<std::pair<std::string, Literal>>::iterator find_in_chain(const std::string& var_name);
            std::vector<std::pair<std::string, Literal>>::iterator find_in_chain(const std::string& var_name, int depth);
            std::vector<std::pair<std::string, Literal>>::iterator end();
            Literal& find_own_or_make(const std::string& var_name);

        private:
            // This is conceptually an unordered map, but prefer vector as default container
            std::vector<std::pair<std::string, Literal>> values_;
            Environment* enclosed_ {};
    };
}}
