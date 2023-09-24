#pragma once

#include <string>
#include <variant>

#include "memory.hpp"
#include "object-fwd.hpp"

namespace motts { namespace lox
{
    using Dynamic_type_value = std::variant<
        // `nullptr_t` should be first so it will be picked as the default constructed value.
        // All others can be listed in any order.
        std::nullptr_t,

        // Primitive types.
        bool,
        double,
        std::string,

        // Object types.
        GC_ptr<Bound_method>,
        GC_ptr<Class>,
        GC_ptr<Closure>,
        GC_ptr<Function>,
        GC_ptr<Instance>
    > ;

    std::ostream& operator<<(std::ostream&, const Dynamic_type_value&);

    struct Is_truthy_visitor
    {
        auto operator()(std::nullptr_t) const
        {
            return false;
        }

        auto operator()(bool value) const
        {
            return value;
        }

        template<typename T>
            auto operator()(const T&) const
            {
                return true;
            }
    };

    struct Mark_objects_visitor
    {
        GC_heap& gc_heap;

        Mark_objects_visitor(GC_heap& gc_heap_arg)
            : gc_heap {gc_heap_arg}
        {
        }

        template<typename T>
            auto operator()(GC_ptr<T> object_type)
            {
                mark(gc_heap, object_type);
            }

        template<typename T>
            auto operator()(const T& /* primitive_type */)
            {
                // No-op.
            }
    };
}}
