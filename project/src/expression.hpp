#pragma once

// Related header
// C standard headers
// C++ standard headers
#include <memory>
// Third-party headers
// This project's headers
#include "exception.hpp"
#include "expression_visitor_fwd.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    struct Expr {
        /*
        Here we run into another Java-to-C++ conundrum. We want the `accept` method to be able to return any type. After
        all, a printer visitor will want to return a string, or a linter visitor may want to return a list. In Nystrom's
        Java code, he achieved this by making the method -- just the method -- generic, where the return type was a type
        variable.

        C++, of course, also has generic templated member functions, except that virtual functions -- as `accept` would
        need to be -- cannot be templated. Which makes sense; a virtual function is ultimately a function pointer, so if
        a virtual function could be templated, then a class would contain some unknown number of function pointers. So
        how does Java pull this off?

        Turns out Java's type variables de-sugar to `Object`. That is, given the Java method declaration `<R> R
        accept(Visitor<R> visitor);`, rather than generating a new method for each type as C++ templates do, Java
        instead implements just a single method as if it were `Object accept(Visitor visitor);`.

        So we face the same problem we did in token.hpp. Java's `Object` can be assigned any value of any type, whereas
        C++ has no universal base class. In an earlier commit, I settled on Boost.Any. It seemed to most closely match
        the Java implementation; it's ultimately a pointer to an arbitrary heap object. Later I decided to shift toward
        static types, where the type correctness is provable at compile-time. The new solution below matches the
        implementation found in the GoF book. The `accept` methods return void, and instead the visitor accumulates the
        result in its private state.
        */
        virtual void accept(Expr_visitor&) const = 0;

        virtual std::unique_ptr<Expr> clone() const = 0;

        // To avoid dynamic_cast tests, implement lvalue-ness polymorphically
        virtual Token&& lvalue_name(const Runtime_error& throwable_if_not_lvalue) &&;

        // Base class boilerplate
        explicit Expr() = default;
        virtual ~Expr() = default;
        Expr(const Expr&) = delete;
        Expr& operator=(const Expr&) = delete;
        Expr(Expr&&) = delete;
        Expr& operator=(Expr&&) = delete;
    };
}}
