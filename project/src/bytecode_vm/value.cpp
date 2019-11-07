#include "value.hpp"

#include <ios>
#include <iostream>
#include <ostream>

using std::boolalpha;
using std::cout;
using std::nullptr_t;
using std::ostream;

using boost::apply_visitor;
using boost::static_visitor;

namespace {
    struct Ostream_visitor : static_visitor<void> {
        ostream& os;

        explicit Ostream_visitor(ostream& os_arg) :
            os {os_arg}
        {}

        auto operator()(double value) {
            os << value;
        }

        auto operator()(bool value) {
            os << boolalpha << value;
        }

        auto operator()(nullptr_t) {
            os << "nil";
        }
    };
}

namespace motts { namespace lox {
    void print_value(Value value) {
        Ostream_visitor ostream_visitor {cout};
        apply_visitor(ostream_visitor, value.variant);
    }
}}
