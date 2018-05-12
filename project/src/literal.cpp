// Related header
#include "literal.hpp"
// C standard headers
// C++ standard headers
#include <ios>
// Third-party headers
// This project's headers
#include "callable.hpp"
#include "class.hpp"

using std::boolalpha;
using std::nullptr_t;
using std::ostream;
using std::shared_ptr;
using std::string;

using boost::apply_visitor;
using boost::static_visitor;

namespace motts { namespace lox {
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

        auto operator()(const shared_ptr<Callable>& callable) {
            os << callable->to_string();
        }

        auto operator()(const shared_ptr<Instance>& instance) {
            os << instance->to_string();
        }
    };

    ostream& operator<<(ostream& os, const Literal& literal) {
        Ostream_visitor ostream_visitor {os};
        apply_visitor(ostream_visitor, literal.value);

        return os;
    }
}}
