#include "value.hpp"

#include "object.hpp"

// Not exported (internal linkage)
namespace {
    // Allow the internal linkage section to access names
    using namespace motts::lox;

    struct Dynamic_type_value_print_visitor {
        std::ostream& os;

        Dynamic_type_value_print_visitor(std::ostream& os_arg)
            : os {os_arg}
        {}

        auto operator()(std::nullptr_t) {
            os << "nil";
        }

        auto operator()(bool value) {
            os << std::boolalpha << value;
        }

        auto operator()(const GC_ptr<Function>& fn) {
            os << "<fn " << fn->name << ">";
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
        std::visit(Dynamic_type_value_print_visitor{os}, value.variant);
        return os;
    }
}}
