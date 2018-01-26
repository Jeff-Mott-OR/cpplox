#pragma once

#include <memory>
#include <string>

#include <boost/any.hpp>
#include <boost/lexical_cast.hpp>

#include "token.hpp"

namespace motts { namespace lox {
    struct Expr_visitor;

    struct Expr {
        explicit Expr();

        /*
        Here we run into another Java-to-C++ conundrum. We want the `accept` method to be able to return any type.
        After all, a printer visitor will want to return a string, a validator visitor may want to return a boolean,
        or a linter visitor could return a list of strings. In Nystrom's Java code, he achieved this
        by making the method -- just the method -- generic, where the return type was a type variable.

        C++, of course, also has generic templated member functions, except that virtual functions --
        as `accept` would need to be -- cannot be templated. Which makes sense; a virtual function is ultimately
        a function pointer, so if a virtual function could be templated, then a class would contain some unknown
        number of function pointers. So how does Java pull this off?

        After some reading and asking some questions, I discovered that Java's type variables de-sugar to `Object`.
        That is, given the Java method declaration `<R> R accept(Visitor<R> visitor);`, rather than generating
        a new method for each type as C++ templates do, Java instead implements just a single method as if it were
        `Object accept(Visitor<Object> visitor);`.

        So we face the same problem we did in token.hpp. Java's `Object` can be assigned any value of any type,
        whereas C++ has no universal base class. And I have to choose between the same set of solutions:
        Boost.Any, Boost.Variant, or templates. Variant is out because I won't know the types in advance.
        Templates offer the best type safety, though I would have to templatize whole hierarchies --
        the `Expr` hierarchy and the `Expr_visitor` hierarchy. Boost.Any seems to most closely match
        the Java implementation. It's ultimately a pointer to an arbitrary heap object. Boost.Any is still type safe,
        though the checks get pushed from compile-time to runtime.
        */

        virtual boost::any accept(const Expr_visitor& visitor) const = 0;

        // Base class boilerplate
        virtual ~Expr();
        Expr(const Expr&) = delete;
        Expr& operator=(const Expr&) = delete;
        Expr(Expr&&) = delete;
        Expr& operator=(Expr&&) = delete;
    };

    struct Binary_expr : Expr {
        std::unique_ptr<Expr> left;
        std::unique_ptr<Token> op;
        std::unique_ptr<Expr> right;

        Binary_expr(
            std::unique_ptr<Expr>&& left_,
            std::unique_ptr<Token>&& op_,
            std::unique_ptr<Expr>&& right_
        );
        boost::any accept(const Expr_visitor& visitor) const override;
    };

    struct Grouping_expr : Expr {
        std::unique_ptr<Expr> expr;

        explicit Grouping_expr(std::unique_ptr<Expr>&& expr_);
        boost::any accept(const Expr_visitor& visitor) const override;
    };

    // As in token.hpp, literal values need a polymorphic implementation.
    // Since I've implemented this twice now, consider making some sort of `Literal_box<T>`
    // and reuse it in both token.hpp and here.
    struct Literal_expr : Expr {
        explicit Literal_expr();
        boost::any accept(const Expr_visitor& visitor) const override;
        virtual std::string to_string() const = 0;
    };

    template<typename Literal_type>
        struct Literal_expr_value : Literal_expr {
            Literal_type value;

            explicit Literal_expr_value(const Literal_type& value_);
            std::string to_string() const override;
        };

    // impl Literal_expr_value

        template<typename Literal_type>
            Literal_expr_value<Literal_type>::Literal_expr_value(const Literal_type& value_)
                : value {value_}
            {}

        template<typename Literal_type>
            std::string Literal_expr_value<Literal_type>::to_string() const {
                return boost::lexical_cast<std::string>(value);
            }

    struct Unary_expr : Expr {
        std::unique_ptr<Token> op;
        std::unique_ptr<Expr> right;

        Unary_expr(std::unique_ptr<Token>&& op_, std::unique_ptr<Expr>&& right_);
        boost::any accept(const Expr_visitor& visitor) const override;
    };

    struct Expr_visitor {
        explicit Expr_visitor();
        virtual boost::any visit_binary(const Binary_expr&) const = 0;
        virtual boost::any visit_grouping(const Grouping_expr&) const = 0;
        virtual boost::any visit_literal(const Literal_expr&) const = 0;
        virtual boost::any visit_unary(const Unary_expr&) const = 0;

        // Base class boilerplate
        virtual ~Expr_visitor();
        Expr_visitor(const Expr_visitor&) = delete;
        Expr_visitor& operator=(const Expr_visitor&) = delete;
        Expr_visitor(Expr_visitor&&) = delete;
        Expr_visitor& operator=(Expr_visitor&&) = delete;
    };
}}
