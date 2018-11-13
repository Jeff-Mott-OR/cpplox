#pragma once

#include <string>
#include <vector>

#include "callable.hpp"
#include "class.hpp"
#include "environment.hpp"
#include "literal.hpp"
#include "statement_impls.hpp"

namespace motts { namespace lox {
    class Function : public Callable {
        public:
            explicit Function(
                const Function_stmt* declaration,
                Environment* enclosed,
                bool is_initializer = false
            );
            Literal call(
                const Callable* /*owner_this*/,
                Interpreter& interpreter,
                const std::vector<Literal>& arguments
            ) const override;
            int arity() const override;
            std::string to_string() const override;
            Function* bind(Instance*) const;

        private:
            const Function_stmt* declaration_ {};
            Environment* enclosed_ {};
            bool is_initializer_;
    };
}}
