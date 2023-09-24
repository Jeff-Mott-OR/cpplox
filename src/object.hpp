#pragma once

#include "object-fwd.hpp"

#include <optional>
#include <string_view>
#include <unordered_map>

#include "chunk.hpp"
#include "memory.hpp"

namespace motts { namespace lox {
    struct Function {
        std::string_view name;
        int arity {0};
        Chunk chunk;
    };

    template<> void trace_refs_trait(GC_heap&, const Function&);

    class Upvalue {
        std::optional<Dynamic_type_value> closed_;

        public:
            std::vector<Dynamic_type_value>& stack;
            const std::vector<Dynamic_type_value>::size_type stack_index;

            Upvalue(std::vector<Dynamic_type_value>& stack, std::vector<Dynamic_type_value>::size_type stack_index);

            const Dynamic_type_value& value() const;
            Dynamic_type_value& value();

            void close();
    };

    template<> void trace_refs_trait(GC_heap&, const Upvalue&);

    struct Closure {
        GC_ptr<Function> function;
        std::vector<GC_ptr<Upvalue>> upvalues;
        std::vector<GC_ptr<Upvalue>> open_upvalues;

        Closure(const GC_ptr<Function>&);
    };

    template<> void trace_refs_trait(GC_heap&, const Closure&);

    struct Class {
        std::string_view name;
        std::unordered_map<std::string, GC_ptr<Closure>> methods;

        Class(const std::string_view& name);
    };

    template<> void trace_refs_trait(GC_heap&, const Class&);

    struct Instance {
        GC_ptr<Class> class_;
        std::unordered_map<std::string, Dynamic_type_value> fields;

        Instance(const GC_ptr<Class>&);
    };

    template<> void trace_refs_trait(GC_heap&, const Instance&);

    struct Bound_method {
        GC_ptr<Instance> instance;
        GC_ptr<Closure> closure;
    };

    template<> void trace_refs_trait(GC_heap&, const Bound_method&);
}}
