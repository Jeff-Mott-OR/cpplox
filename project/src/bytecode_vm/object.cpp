#include "object.hpp"

namespace motts { namespace lox {
    Function::Function(const GC_ptr<std::string>& name_arg) :
        name {name_arg}
    {
    }

    std::ostream& operator<<(std::ostream& os, const Function& fn) {
        if (fn.name) {
            os << "<fn " << *fn.name << ">";
        } else {
            os << "<script>";
        }

        return os;
    }

    template<>
        void trace_refs_trait(GC_heap& gc_heap, const Function& fn) {
            gc_heap.mark(fn.name);
            for (const auto& value : fn.chunk.constants) {
                boost::apply_visitor(Mark_value_visitor{gc_heap}, value.variant);
            }
        }

    Closure::Closure(const GC_ptr<Function>& function_arg) :
        function {function_arg}
    {
    }

    template<>
        void trace_refs_trait(GC_heap& gc_heap, const Closure& closure) {
            gc_heap.mark(closure.function);
            for (const auto& upvalue : closure.upvalues) {
                gc_heap.mark(upvalue);
            }
        }

    Upvalue::Upvalue(std::vector<Value>& stack_arg, std::vector<Value>::size_type position_arg) :
        stack {stack_arg},
        position {position_arg}
    {
    }

    const Value& Upvalue::value() const {
        if (closed_) {
            return *closed_;
        }
        return stack.at(position);
    }

    Value& Upvalue::value() {
        return const_cast<Value&>(const_cast<const Upvalue&>(*this).value());
    }

    void Upvalue::close() {
        closed_ = stack.at(position);
    }

    template<>
        void trace_refs_trait(GC_heap& gc_heap, const Upvalue& upvalue) {
            boost::apply_visitor(Mark_value_visitor{gc_heap}, upvalue.value().variant);
        }

    Class::Class(const GC_ptr<std::string>& name_arg) :
        name {name_arg}
    {
    }

    template<>
        void trace_refs_trait(GC_heap& gc_heap, const Class& klass) {
            gc_heap.mark(klass.name);
            for (const auto& method : klass.methods) {
                gc_heap.mark(method.first);
                boost::apply_visitor(Mark_value_visitor{gc_heap}, method.second.variant);
            }
        }

    Instance::Instance(const GC_ptr<Class>& klass_arg) :
        klass {klass_arg}
    {
    }

    template<>
        void trace_refs_trait(GC_heap& gc_heap, const Instance& instance) {
            gc_heap.mark(instance.klass);
            for (const auto& key_value : instance.fields) {
                gc_heap.mark(key_value.first);
                boost::apply_visitor(Mark_value_visitor{gc_heap}, key_value.second.variant);
            }
        }

    template<>
        void trace_refs_trait(GC_heap& gc_heap, const Bound_method& bound) {
            boost::apply_visitor(Mark_value_visitor{gc_heap}, bound.this_.variant);
            gc_heap.mark(bound.method);
        }
}}
