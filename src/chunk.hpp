#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <variant>
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

    enum struct Opcode
    {
#define X(name) name,
        MOTTS_LOX_OPCODE_NAMES
#undef X
    };

    std::ostream& operator<<(std::ostream&, Opcode);

    struct Source_map_token
    {
        GC_ptr<const std::string> lexeme;
        unsigned int line;
    };

    // Wrapping an int in another type let's us distinguish between them in a variant.
    // Alternatively, we could have used a boolean to discriminate the int's meaning, but this lets us
    // use the type system to ensure correctness, and it still compiles to the same binary.
    // An upvalue is an index into the parent scope's list of locals,
    // and upupvalue is an index into the parent scope's list of upvalues.
    struct Upvalue_index
    {
        unsigned int enclosing_locals_index;

        bool operator==(const Upvalue_index&) const = default;
    };

    struct UpUpvalue_index
    {
        unsigned int enclosing_upvalues_index;

        bool operator==(const UpUpvalue_index&) const = default;
    };

    using Tracked_upvalue = std::variant<Upvalue_index, UpUpvalue_index>;

    // A chunk of bytecode.
    class Chunk
    {
        std::vector<std::uint8_t> bytecode_;
        std::vector<Dynamic_type_value> constants_;
        std::vector<Source_map_token> source_map_tokens_;

        // When we need to patch previous bytecode with a jump distance, then use `Jump_backpatch` to
        // remember the position of the jump instruction and to apply the patch.
        class Jump_backpatch
        {
            std::vector<std::uint8_t>& bytecode_;
            const std::size_t jump_begin_index_;

          public:
            // At the moment of construction, the chunk's bytecode vector is expected to end with two dummy bytes.
            // The constructor will remember the position of those two dummy bytes.
            Jump_backpatch(std::vector<std::uint8_t>& bytecode);

            // Calculate the jump distance from the dummy bytes to the current end of the bytecode.
            // The dummy bytes will be patched with the distance calculated.
            void to_next_opcode();
        };

        // Insert into the constants vector, with deduplication.
        // Returns the index into the constants vector of the inserted value.
        std::size_t insert_constant(Dynamic_type_value);

        // A private helper to emit a raw byte.
        void emit(std::uint8_t, const Source_map_token&);

      public:
        // Read-only access.
        const auto& bytecode() const
        {
            return bytecode_;
        }

        const auto& constants() const
        {
            return constants_;
        }

        const auto& source_map_tokens() const
        {
            return source_map_tokens_;
        }

        // This template is for simple single-byte opcodes. The cpp file will instantiate the compatible opcodes.
        // Example usage: chunk.emit<Opcode::nil>(token); chunk.emit<Opcode::add>(token);
        template<Opcode>
        void emit(const Source_map_token&);

        // This template is for the *_global/class/method/*_property opcodes. The cpp file will instantiate the compatible opcodes.
        // Example usage: chunk.emit<Opcode::define_global>(global_name, token); chunk.emit<Opcode::get_property>(property_name, token);
        template<Opcode>
        void emit(GC_ptr<const std::string> identifier_name, const Source_map_token&);

        // This template is for the *_local/*_upvalue opcodes. The cpp file will instantiate the compatible opcodes.
        // Example usage: chunk.emit<Opcode::get_local>(2, token); chunk.emit<Opcode::set_upvalue>(7, token);
        template<Opcode>
        void emit(unsigned int index, const Source_map_token&);

        void emit_call(unsigned int arg_count, const Source_map_token&);
        void emit_closure(GC_ptr<Function>, const std::vector<Tracked_upvalue>&, const Source_map_token&);
        void emit_constant(Dynamic_type_value, const Source_map_token&);

        // Use returned backpatch to update the bytecode distance.
        // Example usage:
        //     const auto backpatch = chumk.emit_jump(token);
        //     ...
        //     backpatch.to_next_opcode();
        Jump_backpatch emit_jump(const Source_map_token&);

        // Use returned backpatch to update the bytecode distance.
        // Example usage:
        //     const auto backpatch = chumk.emit_jump_if_false(token);
        //     ...
        //     backpatch.to_next_opcode();
        Jump_backpatch emit_jump_if_false(const Source_map_token&);

        void emit_loop(unsigned int loop_begin_bytecode_index, const Source_map_token&);
    };

    std::ostream& operator<<(std::ostream&, const Chunk&);
}
