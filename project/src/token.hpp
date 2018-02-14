#pragma once

#include <cstddef>

#include <ostream>
#include <string>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace motts { namespace lox {
    /*
    We want to serialize an enum's name, but C++ doesn't generate that for us. This will get better when
    C++ gets reflection and injection (https://youtu.be/4AfRAVcThyA?t=21m33s). But for now settle for
    x-macro technique (https://en.wikipedia.org/wiki/X_Macro).

    The enum field names are intentionally lowercase rather than uppercase. Uppercase isn't meant to
    denote a constant; it's meant to denote a macro. Historically, constants were implemented as macros,
    which is why we became accustomed to uppercasing them, but if we're not writing macros then we
    shouldn't uppercase the name. This manifests as a real practical issue because some other header
    already defined a macro named EOF, so if we try to use EOF as one of our enum field names, bad
    things will happen.
    */
    #define MOTTS_LOX_TOKEN_TYPE_NAMES \
        /* Single char tokens */ \
        X(left_paren) X(right_paren) \
        X(left_brace) X(right_brace) \
        X(comma) X(dot) X(minus) X(plus) X(semicolon) X(slash) X(star) \
        \
        /* One or two char tokens */ \
        X(bang) X(bang_equal) X(equal) X(equal_equal) \
        X(greater) X(greater_equal) X(less) X(less_equal) \
        \
        /* Literals */ \
        X(identifier) X(string) X(number) \
        \
        /* Keywords */ \
        /* These are real reserved words in C++, so mangle their names in some way */ \
        X(and_) X(class_) X(else_) X(false_) X(fun_) X(for_) X(if_) X(nil_) X(or_) \
        X(print_) X(return_) X(super_) X(this_) X(true_) X(var_) X(while_) \
        \
        X(eof)

    enum class Token_type {
        #define X(name) name,
        MOTTS_LOX_TOKEN_TYPE_NAMES
        #undef X
    };

    std::ostream& operator<<(std::ostream&, const Token_type&);

    /*
    In Java, the `Token` field `Object literal` can be assigned any value of any type, and we can
    implicitly convert it to a string without any extra work on our part. This is possible because
    `Object` has an overridable `toString` method, and the Java library comes with pre-defined derived
    types for strings, doubles, bools, and more, whose overridden `toString` will do the right thing.

    Not so in C++. In C++, there is no universal base class from which everything inherits, and no pre-
    defined derived types with overridden `toString` methods.

    We have a few options to choose from, such as Boost.Any, Boost.Variant, and derivation. In an
    earlier commit, I settled on templated types that derive from `Token` with a virtual `to_string`
    function -- which is pretty close to how the Java implementation works. This let me generate derived
    types automatically for any literal type -- current and future -- each of which overrides
    `to_string` to do the right thing for that type, and it let me construct non literal-type tokens
    that don't contain any dummy or empty literal value.

    But as the program progressed, the virtual `to_string` wasn't enough. I needed access to the literal
    value itself, and to get that, I needed to downcast to the concrete type. Functionally, that's easy
    to do; it's just a dynamic_cast. But it's a code smell. It means my type wasn't really polymorphic,
    and derivation was probably the wrong solution.

    The new solution below instead uses Boost.Variant. The good part: `Token` can be allocated on the
    stack now instead of the heap, no more base class boilerplate, and no virtual functions. The bad
    part: It has to explicitly list every kind of literal. If ever I want to support a new kind of
    literal, I'll have to update this code. Also every token has to set aside storage for a literal
    value even if it isn't a literal token.
    */

    // Variant wrapped in struct to avoid ambiguous calls due to ADL
    struct Literal_multi_type {
        boost::variant<std::string, double, bool, std::nullptr_t> value;
    };

    std::ostream& operator<<(std::ostream&, const Literal_multi_type&);

    struct Token {
        Token_type type;
        std::string lexeme;
        boost::optional<Literal_multi_type> literal;
        int line;
    };

    std::ostream& operator<<(std::ostream&, const Token&);
}}
