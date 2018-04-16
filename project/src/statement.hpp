#pragma once

// Related header
// C standard headers
// C++ standard headers
#include <memory>
// Third-party headers
// This project's headers
#include "statement_visitor_fwd.hpp"

namespace motts { namespace lox {
    struct Stmt {
        virtual void accept(Stmt_visitor&) = 0;
        virtual std::unique_ptr<Stmt> clone() const = 0;

        // Base class boilerplate
        explicit Stmt() = default;
        virtual ~Stmt() = default;
        Stmt(const Stmt&) = delete;
        Stmt& operator=(const Stmt&) = delete;
        Stmt(Stmt&&) = delete;
        Stmt& operator=(Stmt&&) = delete;
    };
}}
