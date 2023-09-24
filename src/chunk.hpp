#pragma once

#include <cstddef>
#include <ostream>
#include <vector>

#include "scanner.hpp"
#include "value.hpp"

namespace motts { namespace lox {
    #define MOTTS_LOX_OPCODE_NAMES \
        X(constant) \
        X(nil) \
        X(true_) \
        X(false_) \
        X(add) \
        X(print) \
        X(subtract) \
        X(multiply) \
        X(divide) \
        X(negate) \
        X(not_) \
        X(pop) \
        X(less) \
        X(greater) \
        X(equal) \
        X(jump_if_false) \
        X(jump) \
        X(set_global) \
        X(get_global) \
        X(loop) \
        X(define_global) \
        X(get_local) \
        X(set_local) \
        X(call) \
        X(return_) \
        X(closure) \
        X(get_upvalue) \
        X(set_upvalue) \
        X(close_upvalue)

    enum class Opcode {
        #define X(name) name,
        MOTTS_LOX_OPCODE_NAMES
        #undef X
    };

    std::ostream& operator<<(std::ostream&, const Opcode&);

    class Chunk {
        public:
            using Constants_vector = std::vector<Dynamic_type_value>;
            using Bytecode_vector = std::vector<std::uint8_t>;

            struct Tracked_upvalue {
                bool is_direct_capture;
                unsigned int enclosing_index;
            };

        private:
            Constants_vector constants_;
            Bytecode_vector bytecode_;
            std::vector<Token> source_map_tokens_;

            // When we need to patch previous bytecode with a jump distance, then use Jump_backpatch to
            // remember the position of the jump instruction and to apply the patch.
            class Jump_backpatch {
                Bytecode_vector& bytecode_;
                const Bytecode_vector::size_type jump_begin_index_;

                public:
                    Jump_backpatch(Bytecode_vector&);
                    void to_next_opcode();
            };

            Constants_vector::size_type insert_constant(Dynamic_type_value&& constant_value);

        public:
            // Read-only access.
            const auto& constants() const {
                return constants_;
            }

            const auto& bytecode() const {
                return bytecode_;
            }

            const auto& source_map_tokens() const {
                return source_map_tokens_;
            }

            // This template is useful for simple, single-byte opcodes.
            template<Opcode> void emit(const Token& source_map_token);

            // This template is for the *_global opcodes.
            template<Opcode> void emit(const Token& variable_name, const Token& source_map_token);

            // This template is for the get/set local/upvalue opcodes.
            template<Opcode> void emit(int local_stack_index, const Token& source_map_token);

            void emit_call(int arg_count, const Token& source_map_token);
            void emit_closure(const GC_ptr<Function>&, const std::vector<Tracked_upvalue>&, const Token& source_map_token);
            void emit_constant(Dynamic_type_value&& constant_value, const Token& source_map_token);
            Jump_backpatch emit_jump(const Token& source_map_token);
            Jump_backpatch emit_jump_if_false(const Token& source_map_token);
            void emit_loop(int loop_begin_bytecode_index, const Token& source_map_token);
    };

    std::ostream& operator<<(std::ostream&, const Chunk&);

    bool operator==(const Chunk::Tracked_upvalue&, const Chunk::Tracked_upvalue&);
}}
