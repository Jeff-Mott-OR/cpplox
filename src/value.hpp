#pragma once

#include <string>
#include <variant>

#include "memory.hpp"
#include "object-fwd.hpp"

namespace motts { namespace lox
{
    // This variant will be the size of two CPU words and is safe to pass around by value.
    using Dynamic_type_value = std::variant<
        // `nullptr_t` should be first so it will be picked as the default constructed value.
        // All others can be listed in any order.
        std::nullptr_t,

        // Primitive types.
        bool,
        double,

        // Object types.
        GC_ptr<Bound_method>,
        GC_ptr<Class>,
        GC_ptr<Closure>,
        GC_ptr<Function>,
        GC_ptr<Instance>,
        GC_ptr<Native_fn>,
        GC_ptr<const std::string>
    > ;

    std::ostream& operator<<(std::ostream&, Dynamic_type_value);

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
            auto operator()(T) const
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
            auto operator()(T)
            {
                // No-op.
            }
    };
}}

// Define how to hash a GC_ptr of string, which will just be a hash of the underlying string.
namespace std
{
    template<>
        struct hash<motts::lox::GC_ptr<const std::string>>
        {
            std::size_t operator()(motts::lox::GC_ptr<const std::string> str) const
            {
                return std::hash<motts::lox::GC_control_block_base*>{}(str.control_block);
            }
        };
}
