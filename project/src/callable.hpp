#pragma once

#include "callable_fwd.hpp"

#include <string>
#include <vector>

#include "interpreter_fwd.hpp"
#include "literal.hpp"

namespace motts { namespace lox {
    struct Callable {
        virtual Literal call(
            Callable* owner_this,
            Interpreter&,
            const std::vector<Literal>& arguments
        ) = 0;
        virtual int arity() const = 0;
        virtual std::string to_string() const = 0;

        // Base class boilerplate
        explicit Callable() = default;
        virtual ~Callable() = default;
        Callable(const Callable&) = delete;
        Callable& operator=(const Callable&) = delete;
        Callable(Callable&&) = delete;
        Callable& operator=(Callable&&) = delete;
    };
}}
