#pragma once

#pragma warning(push, 0)
    #include <deferred_heap.h>
#pragma warning(pop)

#include "statement_visitor_fwd.hpp"

namespace motts { namespace lox {
    struct Stmt {
        virtual void accept(const gcpp::deferred_ptr<const Stmt>& owner_this, Stmt_visitor&) const = 0;

        // Base class boilerplate
        explicit Stmt() = default;
        virtual ~Stmt() = default;
        Stmt(const Stmt&) = delete;
        Stmt& operator=(const Stmt&) = delete;
        Stmt(Stmt&&) = delete;
        Stmt& operator=(Stmt&&) = delete;
    };
}}
