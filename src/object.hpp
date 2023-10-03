#pragma once

#include "object-fwd.hpp"

#include <span>
#include <unordered_map>
#include <variant>

#include "chunk.hpp"
#include "memory.hpp"

namespace motts { namespace lox
{
    struct Bound_method
    {
        GC_ptr<Instance> instance;
        GC_ptr<Closure> method;
    };

    template<> void trace_refs_trait(GC_heap&, const Bound_method&);

    struct Class
    {
        GC_ptr<const std::string> name;
        std::unordered_map<GC_ptr<const std::string>, GC_ptr<Closure>> methods;

        Class(GC_ptr<const std::string> name);
    };

    template<> void trace_refs_trait(GC_heap&, const Class&);

    struct Closure
    {
        GC_ptr<Function> function;
        // An upvalue refers to a local variable in an enclosing function that the closure uses.
        std::vector<GC_ptr<Upvalue>> upvalues;
        // Following Lua, we’ll use "open upvalue" to refer to an upvalue that points to a local variable still on the stack.
        std::vector<GC_ptr<Upvalue>> open_upvalues;

        Closure(GC_ptr<Function>);
    };

    template<> void trace_refs_trait(GC_heap&, const Closure&);

    struct Function
    {
        GC_ptr<const std::string> name;
        unsigned int arity {0};
        Chunk chunk;
    };

    template<> void trace_refs_trait(GC_heap&, const Function&);

    struct Instance
    {
        GC_ptr<Class> klass;
        std::unordered_map<GC_ptr<const std::string>, Dynamic_type_value> fields;

        Instance(GC_ptr<Class>);
    };

    template<> void trace_refs_trait(GC_heap&, const Instance&);

    struct Native_fn
    {
        Dynamic_type_value (*fn)(std::span<Dynamic_type_value> args);
    };

    class Upvalue
    {
        struct Open
        {
            // These two fields can be thought of as an iterator into the stack,
            // but iterators can be invalidated, so instead keep a stack reference and index.
            std::vector<Dynamic_type_value>& stack;
            const std::vector<Dynamic_type_value>::size_type stack_index;
        };

        struct Closed
        {
            Dynamic_type_value value;
        };

        std::variant<Open, Closed> value_;

        public:
            Upvalue(std::vector<Dynamic_type_value>& stack, std::vector<Dynamic_type_value>::size_type stack_index);

            void close();
            std::vector<Dynamic_type_value>::size_type stack_index() const;
            const Dynamic_type_value& value() const;
            Dynamic_type_value& value();
    };

    template<> void trace_refs_trait(GC_heap&, const Upvalue&);
}}
