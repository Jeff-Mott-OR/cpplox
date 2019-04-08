#include "literal.hpp"

#include <ios>

#include "callable.hpp"
#include "class.hpp"
#include "function.hpp"

using std::boolalpha;
using std::nullptr_t;
using std::ostream;
using std::string;

using boost::apply_visitor;
using boost::static_visitor;
using gcpp::deferred_ptr;

// Allow the internal linkage section to access names
using namespace motts::lox;

// Not exported (internal linkage)
namespace {
    struct Ostream_visitor : static_visitor<void> {
        ostream& os;

        explicit Ostream_visitor(ostream& os_arg) :
            os {os_arg}
        {}

        auto operator()(const string& value) {
            os << value;
        }

        auto operator()(double value) {
            os << value;
        }

        auto operator()(bool value) {
            os << boolalpha << value;
        }

        auto operator()(nullptr_t) {
            os << "nil";
        }

        auto operator()(const deferred_ptr<Callable>& callable) {
            os << callable->to_string();
        }

        auto operator()(const deferred_ptr<Function>& callable) {
            os << callable->to_string();
        }

        auto operator()(const deferred_ptr<Class>& callable) {
            os << callable->to_string();
        }

        auto operator()(const deferred_ptr<Instance>& instance) {
            os << instance->to_string();
        }
    };
}

// Exported (external linkage)
namespace motts { namespace lox {
    ostream& operator<<(ostream& os, const Literal& literal) {
        Ostream_visitor ostream_visitor {os};
        apply_visitor(ostream_visitor, literal.value);

        return os;
    }
}}
