#pragma once

#include "exception.hpp"
#include "expression_visitor_fwd.hpp"
#include "token.hpp"

namespace motts { namespace lox {
    struct Expr {
        /*
        Here we run into a couple other Java-to-C++ conundrums.

        1.
            We want the `accept` method to be able to return any type. After all, a printer visitor will want to return
            a string, but a linter visitor may want to return a list. In Nystrom's Java code, he achieved this by making
            the method -- just the method -- generic, where the return type was a type variable.

            C++, of course, also has generic templated member functions, except that virtual functions -- as `accept`
            would need to be -- cannot be templated. Which makes sense; a virtual function is ultimately a function
            pointer, so if a virtual function could be templated, then a class would contain some unknown number of
            function pointers. So how does Java pull this off?

            Turns out Java's type variables de-sugar to `Object`. That is, given the Java method declaration
            `<R> R accept(Visitor<R> visitor);`, rather than generating a new method for each type as C++ templates do,
            Java instead implements just a single method as if it were `Object accept(Visitor visitor);`.

            So we face the same problem we did in token.hpp. Java's `Object` can be assigned any value of any type,
            whereas C++ has no universal base class. In an earlier commit, I settled on Boost.Any. It seemed to most
            closely match the Java implementation; it's ultimately a pointer to an arbitrary heap object. Later I
            decided to shift toward static types, where the type correctness is provable at compile-time. The new
            solution below matches the implementation found in the GoF book. That is, the `accept` methods return void,
            and instead the visitor accumulates the result in its private state.

        2.
            One thing Java and C++ have in common is if we invoke `accept` on a base `Expr`, then `this` inside each
            `accept` definition will nonetheless be of the appropriate derived type. But what Java and C++ do not have
            in common is the ownership of `this`.

            In Java, `this` is still tracked by the garbage collector, so if the definition of `accept` passes `this` to
            the visitor, then the collector is aware that there's another reference out there. But in C++, `this` is a
            non-owning pointer. If there's an owning smart pointer out there, then `accept` doesn't know about it.

            That turns out to be a problem, because the definition of `accept` needs to be able to pass shared ownership
            to the visitor. So even though the compiler already generates an implicit `this` parameter, we still need an
            `owner_this` smart pointer parameter, and the definition of `accept` will need to cast that `owner_this` to
            the derived type that we know it really is.
        */
        virtual void accept(const Expr* owner_this, Expr_visitor&) const = 0;

        // Some derived expression types can be lvalues; some can't. To avoid
        // dynamic_cast tests, implement lvalue-ness polymorphically.
        virtual const Expr* make_assignment_expression(
            const Expr* lhs_expr,
            const Expr* rhs_expr,
            const Runtime_error& throwable_if_not_lvalue
        ) const;

        // Base class boilerplate
        explicit Expr() = default;
        virtual ~Expr() = default;
        Expr(const Expr&) = delete;
        Expr& operator=(const Expr&) = delete;
        Expr(Expr&&) = delete;
        Expr& operator=(Expr&&) = delete;
    };
}}
