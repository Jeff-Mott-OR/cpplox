#pragma once

#include <string>
#include <utility>
#include <vector>

#pragma warning(push, 0)
    #include <deferred_heap.h>
#pragma warning(pop)

#include "literal.hpp"

namespace motts { namespace lox {
    class Environment {
        public:
            using iterator = std::vector<std::pair<std::string, Literal>>::iterator;

            explicit Environment();
            explicit Environment(const gcpp::deferred_ptr<Environment>& enclosed);
            iterator find_in_chain(const std::string& var_name);
            iterator find_in_chain(const std::string& var_name, int depth);
            iterator end();
            Literal& find_own_or_make(const std::string& var_name);

        private:
            // This is conceptually an unordered map, but prefer vector as default container
            std::vector<std::pair<std::string, Literal>> values_;
            gcpp::deferred_ptr<Environment> enclosed_;
    };
}}
