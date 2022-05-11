#include "value.hpp"

#include <ostream>
#include <string>

#include "object.hpp"

namespace {
    std::ostream& operator<<(std::ostream& os, ObjFunction* function) {
        if (function->name) {
            os << "<fn " << function->name->str << ">";
        } else {
            os << "<script>";
        }

        return os;
    }

    struct Ostream_visitor : boost::static_visitor<void> {
        std::ostream& os;
        explicit Ostream_visitor(std::ostream& os_arg) :
            os {os_arg}
        {}

        auto operator()(bool value) {
            os << std::boolalpha << value;
        }

        auto operator()(std::nullptr_t) {
            os << "nil";
        }

        auto operator()(double value) {
            os << value;
        }

        auto operator()(ObjClass* klass) {
            os << klass->name->str;
        }

        auto operator()(ObjInstance* instance) {
            os << instance->klass->name->str << " instance";
        }

        auto operator()(ObjFunction* function) {
            os << function;
        }

        auto operator()(ObjClosure* closure) {
            os << closure->function;
        }

        auto operator()(ObjBoundMethod* bound) {
            os << bound->method->function;
        }

        auto operator()(ObjNative*) {
            os << "<native fn>";
        }

        auto operator()(ObjString* string) {
            os << string->str;
        }

        auto operator()(ObjUpvalue*) {
            os << "upvalue";
        }
    };
}

bool operator==(const Value& a, const Value& b) {
    return a.variant == b.variant;
}

std::ostream& operator<<(std::ostream& os, const Value& value) {
    Ostream_visitor ostream_visitor {os};
    boost::apply_visitor(ostream_visitor, value.variant);
    return os;
}
