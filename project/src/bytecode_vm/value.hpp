#pragma once

#include <cstddef>
#include <ostream>

#include <boost/variant.hpp>

#include "memory.hpp"
#include "object_fwd.hpp"

namespace motts { namespace lox {
    // A value has the same size and cost as two pointers (a pointer plus a tag)
    struct Value {
        boost::variant<
            std::nullptr_t,
            double,
            bool,
            GC_ptr<std::string>,
            GC_ptr<Native_fn>,
            GC_ptr<Function>,
            GC_ptr<Closure>,
            GC_ptr<Upvalue>,
            GC_ptr<Class>,
            GC_ptr<Instance>,
            GC_ptr<Bound_method>
        > variant;
    };

    bool operator==(const Value&, const Value&);

    std::ostream& operator<<(std::ostream&, const Value&);

    struct Mark_value_visitor : boost::static_visitor<void> {
        GC_heap& gc_heap;

        Mark_value_visitor(GC_heap& gc_heap_arg) :
            gc_heap {gc_heap_arg}
        {
        }

        // Primitives don't need to be marked
        auto operator()(std::nullptr_t) const {}
        auto operator()(double) const {}
        auto operator()(bool) const {}

        // All other object types get marked
        template<typename T>
            auto operator()(const GC_ptr<T>& ptr) const {
                gc_heap.mark(ptr);
            }

        template<typename T>
            auto operator()(const T&) const {
                throw std::logic_error{"Unreachable"};
            }
    };
}}
