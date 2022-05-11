#include "value.hpp"

#include <iomanip>
#include <gsl/gsl_assert>

#include "object.hpp"

namespace motts { namespace lox {
    bool operator==(const Value& a, const Value& b) {
        return a.variant == b.variant;
    }

    struct Ostream_visitor : boost::static_visitor<void> {
        std::ostream& os;

        Ostream_visitor(std::ostream& os_arg) :
            os {os_arg}
        {
        }

        auto operator()(std::nullptr_t) const {
            os << "nil";
        }

        auto operator()(double value) const {
            os << value;
        }

        auto operator()(bool value) const {
            os << std::boolalpha << value;
        }

        auto operator()(const GC_ptr<std::string>& string) const {
            Expects(string);
            os << *string;
        }

        auto operator()(const GC_ptr<Native_fn>&) const {
            os << "<native fn>";
        }

        auto operator()(const GC_ptr<Function>& fn) const {
            Expects(fn);
            os << *fn;
        }

        auto operator()(const GC_ptr<Closure>& closure) const {
            Expects(closure && closure->function);
            os << *closure->function;
        }

        auto operator()(const GC_ptr<Upvalue>&) const {
            os << "upvalue";
        }

        auto operator()(const GC_ptr<Class>& klass) const {
            Expects(klass && klass->name);
            os << *klass->name;
        }

        auto operator()(const GC_ptr<Instance>& instance) const {
            Expects(instance && instance->klass && instance->klass->name);
            os << *instance->klass->name << " instance";
        }

        auto operator()(const GC_ptr<Bound_method>& bound) const {
            Expects(bound && bound->method && bound->method->function);
            os << *bound->method->function;
        }

    };

    std::ostream& operator<<(std::ostream& os, const Value& value) {
        boost::apply_visitor(Ostream_visitor{os}, value.variant);
        return os;
    }
}}
