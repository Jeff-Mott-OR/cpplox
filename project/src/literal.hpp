#pragma once

// Related header
// C standard headers
#include <cstddef>
// C++ standard headers
#include <memory>
#include <ostream>
#include <string>
// Third-party headers
#include <boost/variant.hpp>
// This project's headers
#include "callable_fwd.hpp"

namespace motts { namespace lox {
    /*
    In Java, the `Token` field `Object literal` can be assigned any value of any type, and we can implicitly convert it
    to a string without any extra work on our part. This is possible because `Object` has an overridable `toString`
    method, and the Java library comes with pre-defined derived types for strings, doubles, bools, and more, whose
    overridden `toString` will do the right thing.

    Not so in C++. In C++, there is no universal base class from which everything inherits, and no pre-defined derived
    types with overridden `toString` methods.

    We have a few options to choose from, such as Boost.Any, Boost.Variant, and derivation. In an earlier commit, I
    settled on templated types that derive from `Token` with a virtual `to_string` function -- which is pretty close to
    how the Java implementation works. This let me generate derived types automatically for any literal type -- current
    and future -- each of which overrides `to_string` to do the right thing for that type, and it let me construct non
    literal-type tokens that don't contain any dummy or empty literal value.

    But as the program progressed, the virtual `to_string` wasn't enough. I needed access to the literal value itself,
    and to get that, I needed to downcast to the concrete type. Functionally, that's easy to do; it's just a
    dynamic_cast. But it's a code smell. It means my type wasn't really polymorphic, and derivation was probably the
    wrong solution.

    The new solution below instead uses Boost.Variant. The good part: `Token` can be allocated on the stack now instead
    of the heap, no more base class boilerplate, and no virtual functions. The bad part: It has to explicitly list every
    kind of literal. If ever I want to support a new kind of literal, I'll have to update this code. Also every token
    has to set aside storage for a literal value even if it isn't a literal token.
    */

    // Variant wrapped in struct to avoid ambiguous calls due to ADL
    struct Literal {
        boost::variant<std::nullptr_t, std::string, double, bool, std::shared_ptr<Callable>> value;
    };

    std::ostream& operator<<(std::ostream&, const Literal&);
}}
