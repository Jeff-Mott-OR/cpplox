#include "object.hpp"

namespace motts { namespace lox {
    template<>
        void trace_refs_trait(GC_heap& gc_heap, const Function& function) {
            for (const auto& value : function.chunk.constants()) {
                std::visit(Dynamic_type_value::Mark_objects_visitor{gc_heap}, value.variant);
            }
        }

    Upvalue::Upvalue(std::vector<Dynamic_type_value>& stack_arg, std::vector<Dynamic_type_value>::size_type stack_index_arg)
        : stack {stack_arg},
          stack_index {stack_index_arg}
    {}

    const Dynamic_type_value& Upvalue::value() const {
        if (closed_) {
            return *closed_;
        }
        return stack.at(stack_index);
    }

    Dynamic_type_value& Upvalue::value() {
        return const_cast<Dynamic_type_value&>(const_cast<const Upvalue&>(*this).value());
    }

    void Upvalue::close() {
        if (! closed_) {
            closed_ = stack.at(stack_index);
        }
    }

    template<>
        void trace_refs_trait(GC_heap& gc_heap, const Upvalue& upvalue) {
            std::visit(Dynamic_type_value::Mark_objects_visitor{gc_heap}, upvalue.value().variant);
        }

    Closure::Closure(const GC_ptr<Function>& function_arg)
        : function {function_arg}
    {}

    template<>
        void trace_refs_trait(GC_heap& gc_heap, const Closure& closure) {
            mark(gc_heap, closure.function);

            for (const auto& upvalue : closure.upvalues) {
                mark(gc_heap, upvalue);
            }
            for (const auto& upvalue : closure.open_upvalues) {
                mark(gc_heap, upvalue);
            }
        }
}}
