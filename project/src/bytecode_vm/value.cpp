#include "value.hpp"

#include <ios>
#include <iostream>
#include <ostream>

#include "function.hpp"

using std::boolalpha;
using std::cout;
using std::nullptr_t;
using std::ostream;
using std::string;

using boost::apply_visitor;
using boost::static_visitor;

using namespace motts::lox;

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

        auto operator()(const Function* fn) {
            if (fn->name.empty()) {
                os << "<script>";
                return;
            }

            os << "<fn " << fn->name << ">";
        }

        auto operator()(const Native_fn*) {
            os << "<native fn>";
        }
    };
}

namespace motts { namespace lox {
    void print_value(const Value& value) {
        Ostream_visitor ostream_visitor {cout};
        apply_visitor(ostream_visitor, value.variant);
    }
}}
