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
                const Function_expr* declaration,
                Environment* enclosed,
                bool is_initializer = false
            );
            Literal call(
                Callable* /*owner_this*/,
                Interpreter& interpreter,
                const std::vector<Literal>& arguments
            ) override;
            int arity() const override;
            std::string to_string() const override;
            Function* bind(Instance*) const;

        private:
            const Function_expr* declaration_ {};
            Environment* enclosed_ {};
            bool is_initializer_;
    };
}}
