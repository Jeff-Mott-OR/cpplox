#pragma once

#include <ostream>
#include <unordered_map>
#include <vector>

#include "chunk.hpp"
#include "interned_strings.hpp"
#include "memory.hpp"
#include "value.hpp"

namespace motts::lox
{
    class VM
    {
        const bool debug_;
        std::ostream& os_;
        GC_heap& gc_heap_;
        Interned_strings& interned_strings_;
        std::size_t gc_heap_last_collect_size_{0};

        std::vector<Dynamic_type_value> stack_;
        std::vector<GC_ptr<Closure>> call_frames_;
        std::unordered_map<GC_ptr<const std::string>, Dynamic_type_value> globals_;

      public:
        VM(GC_heap&, Interned_strings&, std::ostream&, bool debug = false);
        ~VM();

        void run(GC_ptr<Function>);

      private:
        void run(GC_ptr<Closure>, std::size_t stack_begin_index);
        void maybe_collect_garbage();

        // These opcode functions used to be in a big switch statement, but for the purposes of using the perf profiling tools, it's
        // important and helpful to have symbol names, so we can measure how much time is spent in which opcode. I wish I could have solved
        // this problem with reference-capturing lambdas, but those don't provide symbols either for perf reports. So instead I have to
        // manually write out the parameters and arguments.
        using stack_begin_index_t = std::size_t;
        using bytecode_iter_t = std::vector<std::uint8_t>::const_iterator;
        using constants_t = std::vector<Dynamic_type_value>;
        using upvalues_t = std::vector<GC_ptr<Upvalue>>;
        using open_upvalues_t = std::vector<GC_ptr<Upvalue>>;
        using source_map_tokens_t = std::vector<Source_map_token>;
        using bytecode_index_t = std::ptrdiff_t;
        void opcode_add(const source_map_tokens_t&, const bytecode_index_t&);
        void opcode_call(bytecode_iter_t&, const source_map_tokens_t&, const bytecode_index_t&);
        void opcode_class_(bytecode_iter_t&, const constants_t&);
        void opcode_close_upvalue(open_upvalues_t&);
        void opcode_closure(const stack_begin_index_t&, bytecode_iter_t&, const constants_t&, upvalues_t&, open_upvalues_t&);
        void opcode_constant(bytecode_iter_t&, const constants_t&);
        void opcode_divide(const source_map_tokens_t&, const bytecode_index_t&);
        void opcode_equal();
        void opcode_false_();
        void opcode_define_global(bytecode_iter_t&, const constants_t&);
        void opcode_get_global(bytecode_iter_t&, const constants_t&, const source_map_tokens_t&, const bytecode_index_t&);
        void opcode_get_local(const stack_begin_index_t&, bytecode_iter_t&);
        void opcode_get_property(bytecode_iter_t&, const constants_t&, const source_map_tokens_t&, const bytecode_index_t&);
        void opcode_get_super(bytecode_iter_t&, const constants_t&, const source_map_tokens_t&, const bytecode_index_t&);
        void opcode_get_upvalue(bytecode_iter_t&, upvalues_t&);
        void opcode_greater(const source_map_tokens_t&, const bytecode_index_t&);
        void opcode_inherit(const source_map_tokens_t&, const bytecode_index_t&);
        void opcode_invoke(bytecode_iter_t&, const constants_t&, const source_map_tokens_t&, const bytecode_index_t&);
        void opcode_jump_loop(const Opcode&, bytecode_iter_t&);
        void opcode_less(const source_map_tokens_t&, const bytecode_index_t&);
        void opcode_method(bytecode_iter_t&, const constants_t&);
        void opcode_multiply(const source_map_tokens_t&, const bytecode_index_t&);
        void opcode_negate(const source_map_tokens_t&, const bytecode_index_t&);
        void opcode_nil();
        void opcode_not_();
        void opcode_pop();
        void opcode_print();
        void opcode_return_(const stack_begin_index_t&, open_upvalues_t&);
        void opcode_set_global(bytecode_iter_t&, const constants_t&, const source_map_tokens_t&, const bytecode_index_t&);
        void opcode_set_local(const stack_begin_index_t&, bytecode_iter_t&);
        void opcode_set_property(bytecode_iter_t&, const constants_t&, const source_map_tokens_t&, const bytecode_index_t&);
        void opcode_set_upvalue(bytecode_iter_t&, upvalues_t&);
        void opcode_subtract(const source_map_tokens_t&, const bytecode_index_t&);
        void opcode_true_();
    };
}
