#pragma once

#include <memory>

#include "token.hpp"

namespace motts { namespace lox {
    struct Expr_visitor;

    struct Expr {
        explicit Expr();

        /*
        Here we run into another Java-to-C++ conundrum. We want the `accept` method to
        be able to return any type. After all, a printer visitor will want to return a
        string, a validator visitor may want to return a boolean, or a linter visitor
        could return a list of strings. In Nystrom's Java code, he achieved this by
        making the method -- just the method -- generic, where the return type was a
        type variable.

        C++, of course, also has generic templated member functions, except that virtual
        functions -- as `accept` would need to be -- cannot be templated. Which makes
        sense; a virtual function is ultimately a function pointer, so if a virtual
        function could be templated, then a class would contain some unknown number of
        function pointers. So how does Java pull this off?

        After some reading and asking some questions, I discovered that Java's type
        variables de-sugar to `Object`. That is, given the Java method declaration
        `<R> R accept(Visitor<R> visitor);`, rather than generating a new method for each
        type as C++ templates do, Java instead implements just a single method as if it
        were `Object accept(Visitor visitor);`.

        So we face the same problem we did in token.hpp. Java's `Object` can be assigned
        any value of any type, whereas C++ has no universal base class. In an earlier
        commit, I settled on Boost.Any. It seemed to most closely match the Java
        implementation. It's ultimately a pointer to an arbitrary heap object. Later I
        decided to shift toward static types, where the type correctness is provable at
        compile-time. The new solution below matches the implementation found in the GoF
        book. The `accept` methods return void, and instead the visitor accumulates the
        result in its private state.
        */

        virtual void accept(Expr_visitor&) const = 0;

        // Base class boilerplate
        virtual ~Expr();
        Expr(const Expr&) = delete;
        Expr& operator=(const Expr&) = delete;
        Expr(Expr&&) = delete;
        Expr& operator=(Expr&&) = delete;
    };

    struct Binary_expr : Expr {
        std::unique_ptr<Expr> left;
        Token op;
        std::unique_ptr<Expr> right;

        Binary_expr(
            std::unique_ptr<Expr>&& left,
            Token&& op,
            std::unique_ptr<Expr>&& right
        );
        void accept(Expr_visitor&) const override;
    };

    struct Grouping_expr : Expr {
        std::unique_ptr<Expr> expr;

        explicit Grouping_expr(std::unique_ptr<Expr>&& expr);
        void accept(Expr_visitor&) const override;
    };

    struct Literal_expr : Expr {
        Literal_multi_type value;

        explicit Literal_expr(Literal_multi_type&& value);
        void accept(Expr_visitor&) const override;
    };

    struct Unary_expr : Expr {
        Token op;
        std::unique_ptr<Expr> right;

        Unary_expr(Token&& op, std::unique_ptr<Expr>&& right);
        void accept(Expr_visitor&) const override;
    };

    struct Expr_visitor {
        explicit Expr_visitor();

        virtual void visit(const Binary_expr&) = 0;
        virtual void visit(const Grouping_expr&) = 0;
        virtual void visit(const Literal_expr&) = 0;
        virtual void visit(const Unary_expr&) = 0;

        // Base class boilerplate
        virtual ~Expr_visitor();
        Expr_visitor(const Expr_visitor&) = delete;
        Expr_visitor& operator=(const Expr_visitor&) = delete;
        Expr_visitor(Expr_visitor&&) = delete;
        Expr_visitor& operator=(Expr_visitor&&) = delete;
    };
}}
