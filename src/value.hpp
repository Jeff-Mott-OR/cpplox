#pragma once

#include <string>
#include <variant>

#include "memory.hpp"
#include "object-fwd.hpp"

namespace motts { namespace lox {
    struct Dynamic_type_value {
        std::variant<std::nullptr_t, bool, double, std::string, GC_ptr<Function>, GC_ptr<Closure>, GC_ptr<Class>, GC_ptr<Instance>> variant;

        struct Mark_objects_visitor {
            GC_heap& gc_heap;

            Mark_objects_visitor(GC_heap& gc_heap_arg)
                : gc_heap {gc_heap_arg}
            {}

            template<typename T> auto operator()(const GC_ptr<T>& object_type) {
                mark(gc_heap, object_type);
            }

            template<typename T> auto operator()(const T& /* primitive_type */) {
                // No-op.
            }
        };

        struct Truthy_visitor {
            auto operator()(std::nullptr_t) const {
                return false;
            }

            auto operator()(bool value) const {
                return value;
            }

            template<typename T>
                auto operator()(const T&) const {
                    return true;
                }
        };
    };

    bool operator==(const Dynamic_type_value& lhs, const Dynamic_type_value& rhs);

    std::ostream& operator<<(std::ostream&, const Dynamic_type_value&);
}}
