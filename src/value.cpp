#include "value.hpp"

#include "object.hpp"

// Not exported (internal linkage)
namespace {
    // Allow the internal linkage section to access names
    using namespace motts::lox;

    struct Print_visitor {
        std::ostream& os;

        Print_visitor(std::ostream& os_arg)
            : os {os_arg}
        {}

        auto operator()(std::nullptr_t) {
            os << "nil";
        }

        auto operator()(bool value) {
            os << std::boolalpha << value;
        }

        auto operator()(const GC_ptr<Function>& fn) {
            os << "<fn " << fn->name << '>';
        }

        auto operator()(const GC_ptr<Closure>& closure) {
            (*this)(closure->function);
        }

        auto operator()(const GC_ptr<Class>& class_) {
            os << "<class " << class_->name << '>';
        }

        auto operator()(const GC_ptr<Instance>& instance) {
            os << "<instance " << instance->class_->name << '>';
        }

        auto operator()(const GC_ptr<Bound_method>& bound_method) {
            (*this)(bound_method->closure->function);
        }

        template<typename T>
            auto operator()(const T& value) {
                os << value;
            }
    };
}

namespace motts { namespace lox {
    bool operator==(const Dynamic_type_value& lhs, const Dynamic_type_value& rhs) {
        return lhs.variant == rhs.variant;
    }

    std::ostream& operator<<(std::ostream& os, const Dynamic_type_value& value) {
        std::visit(Print_visitor{os}, value.variant);
        return os;
    }
}}
