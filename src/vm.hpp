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

        // These opcode functions used to be in a big switch statement, but for the purposes of using the perf profiling
        // tools, it's important and helpful to have symbol names so we can measure how much time is spent in which
        // opcode. I wish I could have solved this problem with reference-capturing lambdas, but those don't provide
        // symbols either for perf reports. So instead I have to manually write out the parameters and arguments.
        void opcode_add(std::span<const Source_map_token> source_map_tokens, std::ptrdiff_t bytecode_index);
        void opcode_call(
            std::vector<std::uint8_t>::const_iterator& bytecode_iter,
            std::span<const Source_map_token> source_map_tokens,
            std::ptrdiff_t bytecode_index
        );
        void opcode_class_(
            std::vector<std::uint8_t>::const_iterator& bytecode_iter,
            std::span<const Dynamic_type_value> constants
        );
        void opcode_close_upvalue(std::vector<GC_ptr<Upvalue>>& open_upvalues);
        void opcode_closure(
            std::size_t stack_begin_index,
            std::vector<std::uint8_t>::const_iterator& bytecode_iter,
            std::span<const Dynamic_type_value> constants,
            std::span<GC_ptr<Upvalue>> upvalues,
            std::vector<GC_ptr<Upvalue>>& open_upvalues
        );
        void opcode_constant(
            std::vector<std::uint8_t>::const_iterator& bytecode_iter,
            std::span<const Dynamic_type_value> constants
        );
        void opcode_divide(std::span<const Source_map_token> source_map_tokens, std::ptrdiff_t bytecode_index);
        void opcode_equal();
        void opcode_false_();
        void opcode_define_global(
            std::vector<std::uint8_t>::const_iterator& bytecode_iter,
            std::span<const Dynamic_type_value> constants
        );
        void opcode_get_global(
            std::vector<std::uint8_t>::const_iterator& bytecode_iter,
            std::span<const Dynamic_type_value> constants,
            std::span<const Source_map_token> source_map_tokens,
            std::ptrdiff_t bytecode_index
        );
        void opcode_get_local(std::size_t stack_begin_index, std::vector<std::uint8_t>::const_iterator& bytecode_iter);
        void opcode_get_property(
            std::vector<std::uint8_t>::const_iterator& bytecode_iter,
            std::span<const Dynamic_type_value> constants,
            std::span<const Source_map_token> source_map_tokens,
            std::ptrdiff_t bytecode_index
        );
        void opcode_get_super(
            std::vector<std::uint8_t>::const_iterator& bytecode_iter,
            std::span<const Dynamic_type_value> constants,
            std::span<const Source_map_token> source_map_tokens,
            std::ptrdiff_t bytecode_index
        );
        void opcode_get_upvalue(
            std::vector<std::uint8_t>::const_iterator& bytecode_iter,
            std::span<GC_ptr<Upvalue>> upvalues
        );
        void opcode_greater(std::span<const Source_map_token> source_map_tokens, std::ptrdiff_t bytecode_index);
        void opcode_inherit(std::span<const Source_map_token> source_map_tokens, std::ptrdiff_t bytecode_index);
        void opcode_invoke(
            std::vector<std::uint8_t>::const_iterator& bytecode_iter,
            std::span<const Dynamic_type_value> constants,
            std::span<const Source_map_token> source_map_tokens,
            std::ptrdiff_t bytecode_index
        );
        void opcode_jump_loop(const Opcode&, std::vector<std::uint8_t>::const_iterator& bytecode_iter);
        void opcode_less(std::span<const Source_map_token> source_map_tokens, std::ptrdiff_t bytecode_index);
        void opcode_method(
            std::vector<std::uint8_t>::const_iterator& bytecode_iter,
            std::span<const Dynamic_type_value> constants
        );
        void opcode_multiply(std::span<const Source_map_token> source_map_tokens, std::ptrdiff_t bytecode_index);
        void opcode_negate(std::span<const Source_map_token> source_map_tokens, std::ptrdiff_t bytecode_index);
        void opcode_nil();
        void opcode_not_();
        void opcode_pop();
        void opcode_print();
        void opcode_return_(std::size_t stack_begin_index, std::span<GC_ptr<Upvalue>> open_upvalues);
        void opcode_set_global(
            std::vector<std::uint8_t>::const_iterator& bytecode_iter,
            std::span<const Dynamic_type_value> constants,
            std::span<const Source_map_token> source_map_tokens,
            std::ptrdiff_t bytecode_index
        );
        void opcode_set_local(std::size_t stack_begin_index, std::vector<std::uint8_t>::const_iterator& bytecode_iter);
        void opcode_set_property(
            std::vector<std::uint8_t>::const_iterator& bytecode_iter,
            std::span<const Dynamic_type_value> constants,
            std::span<const Source_map_token> source_map_tokens,
            std::ptrdiff_t bytecode_index
        );
        void opcode_set_upvalue(
            std::vector<std::uint8_t>::const_iterator& bytecode_iter,
            std::span<GC_ptr<Upvalue>> upvalues
        );
        void opcode_subtract(std::span<const Source_map_token> source_map_tokens, std::ptrdiff_t bytecode_index);
        void opcode_true_();
    };
}
