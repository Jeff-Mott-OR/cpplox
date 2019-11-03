#pragma once

#include <cstddef>

#include <ostream>
#include <string>

#include <boost/variant.hpp>
#pragma warning(push, 0)
    #include <deferred_heap.h>
#pragma warning(pop)

#include "callable_fwd.hpp"
#include "class_fwd.hpp"
#include "function_fwd.hpp"

namespace motts { namespace lox {
    // Variant wrapped in struct to avoid ambiguous calls due to ADL
    struct Literal {
        boost::variant<
            // It's important that nullptr_t is first; variant default-constructs its first bounded type
            std::nullptr_t,
            std::string,
            double,
            bool,
            gcpp::deferred_ptr<Callable>,
            gcpp::deferred_ptr<Function>,
            gcpp::deferred_ptr<Class>,
            gcpp::deferred_ptr<Instance>
        > value;
    };

    std::ostream& operator<<(std::ostream&, const Literal&);
}}
