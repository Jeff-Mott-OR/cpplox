#pragma once

// Related header
#include "callable_fwd.hpp"
// C standard headers
// C++ standard headers
#include <string>
#include <vector>
// Third-party headers
// This project's headers
#include "interpreter_fwd.hpp"
#include "literal.hpp"

namespace motts { namespace lox {
    struct Callable {
        virtual Literal call(Interpreter&, const std::vector<Literal>& arguments) const = 0;
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
