#pragma once

#include <boost/variant.hpp>

// #include "object_fwd.hpp"
struct ObjString;
struct ObjClass;
struct ObjInstance;
struct ObjFunction;
struct ObjClosure;
struct ObjBoundMethod;
struct ObjNative;
struct ObjUpvalue;

struct Value {
    boost::variant<
        std::nullptr_t,
        bool,
        double,
        ObjClass*,
        ObjInstance*,
        ObjFunction*,
        ObjClosure*,
        ObjBoundMethod*,
        ObjNative*,
        ObjString*,
        ObjUpvalue*
    > variant;
};

bool operator==(const Value& a, const Value& b);
std::ostream& operator<<(std::ostream& os, const Value& value);
