#include "object.hpp"

namespace motts { namespace lox
{
    template<>
        void trace_refs_trait(GC_heap& gc_heap, const Bound_method& bound_method)
        {
            mark(gc_heap, bound_method.instance);
            mark(gc_heap, bound_method.method);
        }

    Class::Class(GC_ptr<const std::string> name_arg)
        : name {name_arg}
    {
    }

    template<>
        void trace_refs_trait(GC_heap& gc_heap, const Class& klass)
        {
            mark(gc_heap, klass.name);
            for (const auto& [key, method] : klass.methods) {
                mark(gc_heap, key);
                mark(gc_heap, method);
            }
        }

    Closure::Closure(GC_ptr<Function> function_arg)
        : function {function_arg}
    {
    }

    template<>
        void trace_refs_trait(GC_heap& gc_heap, const Closure& closure)
        {
            mark(gc_heap, closure.function);
            for (const auto& upvalue : closure.upvalues) {
                mark(gc_heap, upvalue);
            }
            for (const auto& upvalue : closure.open_upvalues) {
                mark(gc_heap, upvalue);
            }
        }

    template<>
        void trace_refs_trait(GC_heap& gc_heap, const Function& function)
        {
            mark(gc_heap, function.name);
            for (const auto& value : function.chunk.constants()) {
                std::visit(Mark_objects_visitor{gc_heap}, value);
            }
            for (const auto& source_map_token : function.chunk.source_map_tokens()) {
                mark(gc_heap, source_map_token.lexeme);
            }
        }

    Instance::Instance(GC_ptr<Class> klass_arg)
        : klass {klass_arg}
    {
    }

    template<>
        void trace_refs_trait(GC_heap& gc_heap, const Instance& instance)
        {
            mark(gc_heap, instance.klass);
            for (const auto& [key, field] : instance.fields) {
                mark(gc_heap, key);
                std::visit(Mark_objects_visitor{gc_heap}, field);
            }
        }

    Upvalue::Upvalue(std::vector<Dynamic_type_value>& stack_arg, std::vector<Dynamic_type_value>::size_type stack_index_arg)
        : value_ {Open{stack_arg, stack_index_arg}}
    {
    }

    void Upvalue::close()
    {
        const auto open = std::get<Open>(value_);
        value_ = Closed{open.stack.at(open.stack_index)};
    }

    std::vector<Dynamic_type_value>::size_type Upvalue::stack_index() const
    {
        return std::get<Open>(value_).stack_index;
    }

    const Dynamic_type_value& Upvalue::value() const
    {
        if (std::holds_alternative<Closed>(value_)) {
            return std::get<Closed>(value_).value;
        }

        const auto& open = std::get<Open>(value_);
        return open.stack.at(open.stack_index);
    }

    Dynamic_type_value& Upvalue::value()
    {
        return const_cast<Dynamic_type_value&>(const_cast<const Upvalue&>(*this).value());
    }

    template<>
        void trace_refs_trait(GC_heap& gc_heap, const Upvalue& upvalue)
        {
            std::visit(Mark_objects_visitor{gc_heap}, upvalue.value());
        }
}}
