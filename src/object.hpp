#pragma once

#include "object-fwd.hpp"

#include <optional>
#include <string_view>

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
}}
