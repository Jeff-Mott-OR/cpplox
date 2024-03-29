#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <vector>

#include "memory.hpp"
#include "scanner.hpp"
#include "value.hpp"

namespace motts::lox
{
// Internally, these opcodes could be listed in any order and work fine.
// But for the generated opcode values to match clox opcodes
// (which isn't necessarily important to do), then this order has to match clox.
// X-macro technique to re-use this list in multiple places.
#define MOTTS_LOX_OPCODE_NAMES \
    X(constant) \
    X(nil) \
    X(true_) \
    X(false_) \
    X(pop) \
    X(get_local) \
    X(set_local) \
    X(get_global) \
    X(define_global) \
    X(set_global) \
    X(get_upvalue) \
    X(set_upvalue) \
    X(get_property) \
    X(set_property) \
    X(get_super) \
    X(equal) \
    X(greater) \
    X(less) \
    X(add) \
    X(subtract) \
    X(multiply) \
    X(divide) \
    X(not_) \
    X(negate) \
    X(print) \
    X(jump) \
    X(jump_if_false) \
    X(loop) \
    X(call) \
    X(invoke) \
    X(super_invoke) \
    X(closure) \
    X(close_upvalue) \
    X(return_) \
    X(class_) \
    X(inherit) \
    X(method)

    enum class Opcode
    {
#define X(name) name,
        MOTTS_LOX_OPCODE_NAMES
#undef X
    };

    std::ostream& operator<<(std::ostream&, Opcode);

    // A chunk of bytecode.
    class Chunk
    {
      public:
        using Constants_vector = std::vector<Dynamic_type_value>;
        using Bytecode_vector = std::vector<std::uint8_t>;

        // Like the scanner token, but this one owns the lexeme string.
        struct Token
        {
            Token_type type;
            GC_ptr<const std::string> lexeme;
            unsigned int line;
        };

        struct Tracked_upvalue
        {
            // This value is either an index into the enclosing locals, if direct capture is true,
            // or it's an index into the enclosing upvalues, if direct capture is false.
            unsigned int enclosing_index;
            bool is_direct_capture;
        };

      private:
        Constants_vector constants_;
        Bytecode_vector bytecode_;
        std::vector<Token> source_map_tokens_;

        // When we need to patch previous bytecode with a jump distance, then use `Jump_backpatch` to
        // remember the position of the jump instruction and to apply the patch.
        class Jump_backpatch
        {
            Bytecode_vector& bytecode_;
            const Bytecode_vector::size_type jump_begin_index_;

          public:
            Jump_backpatch(Bytecode_vector&);
            void to_next_opcode();
        };

        std::size_t insert_constant(Dynamic_type_value);

      public:
        // Read-only access.
        const auto& constants() const
        {
            return constants_;
        }

        // Read-only access.
        const auto& bytecode() const
        {
            return bytecode_;
        }

        // Read-only access.
        const auto& source_map_tokens() const
        {
            return source_map_tokens_;
        }

        // This template is useful for simple, single-byte opcodes.
        template<Opcode>
        void emit(const Token& source_map_token);

        // This template is for the *_global/class/method/*_property opcodes.
        template<Opcode>
        void emit(GC_ptr<const std::string> identifier_name, const Token& source_map_token);

        // This template is for the *_local/*_upvalue opcodes.
        template<Opcode>
        void emit(unsigned int index, const Token& source_map_token);

        void emit_call(unsigned int arg_count, const Token& source_map_token);
        void emit_closure(GC_ptr<Function>, const std::vector<Tracked_upvalue>&, const Token& source_map_token);
        void emit_constant(Dynamic_type_value, const Token& source_map_token);
        Jump_backpatch emit_jump(const Token& source_map_token);
        Jump_backpatch emit_jump_if_false(const Token& source_map_token);
        void emit_loop(unsigned int loop_begin_bytecode_index, const Token& source_map_token);
    };

    std::ostream& operator<<(std::ostream&, const Chunk&);

    bool operator==(Chunk::Tracked_upvalue, Chunk::Tracked_upvalue);
}
