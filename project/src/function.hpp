#pragma once

#include "callable.hpp"
#include "environment.hpp"
#include "interpreter.hpp"
#include "statement_impls.hpp"

namespace motts { namespace lox {
    class Function : public Callable {
        public:
            explicit Function(
                gcpp::deferred_heap&,
                Interpreter&,
                const gcpp::deferred_ptr<const Function_expr>& declaration,
                const gcpp::deferred_ptr<Environment>& enclosed,
                bool is_initializer = false
            );
            Literal call(const gcpp::deferred_ptr<Callable>& owner_this, const std::vector<Literal>& arguments) override;
            int arity() const override;
            std::string to_string() const override;
            gcpp::deferred_ptr<Function> bind(const gcpp::deferred_ptr<Instance>&) const;

        private:
            gcpp::deferred_heap& deferred_heap_;
            Interpreter& interpreter_;
            gcpp::deferred_ptr<const Function_expr> declaration_;
            gcpp::deferred_ptr<Environment> enclosed_;
            bool is_initializer_;
    };
}}
