#pragma once

#include "object_fwd.hpp"

#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "chunk.hpp"
#include "memory.hpp"
#include "value.hpp"

namespace motts { namespace lox {
    struct Native_fn {
        std::function<Value(const std::vector<Value>&)> fn;
    };

    struct Function {
        Chunk chunk;
        GC_ptr<std::string> name;
        int arity {};
        int upvalue_count {};

        Function(const GC_ptr<std::string>& name);
    };

    template<> void trace_refs_trait(GC_heap&, const Function&);

    std::ostream& operator<<(std::ostream&, const Function&);

    struct Closure {
        GC_ptr<Function> function;
        std::vector<GC_ptr<Upvalue>> upvalues;

        Closure(const GC_ptr<Function>&);
    };

    template<> void trace_refs_trait(GC_heap&, const Closure&);

    class Upvalue {
        std::optional<Value> closed_;

        public:
            // Vector iterators and pointers can be invalidated, so instead store a stack reference and position
            std::vector<Value>& stack;
            const std::vector<Value>::size_type position;

            Upvalue(std::vector<Value>& stack, std::vector<Value>::size_type position);

            const Value& value() const;
            Value& value();

            void close();
    };

    template<> void trace_refs_trait(GC_heap&, const Upvalue&);

    struct Class {
        GC_ptr<std::string> name;
        std::unordered_map<GC_ptr<std::string>, Value> methods;

        Class(const GC_ptr<std::string>& name);
    };

    template<> void trace_refs_trait(GC_heap&, const Class&);

    struct Instance {
        GC_ptr<Class> klass;
        std::unordered_map<GC_ptr<std::string>, Value> fields;

        Instance(const GC_ptr<Class>&);
    };

    template<> void trace_refs_trait(GC_heap&, const Instance&);

    struct Bound_method {
        Value this_;
        GC_ptr<Closure> method;
    };

    template<> void trace_refs_trait(GC_heap&, const Bound_method&);
}}
