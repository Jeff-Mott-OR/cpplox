#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "scanner.hpp"

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
        X(equal)

    enum class Opcode {
        #define X(name) name,
        MOTTS_LOX_OPCODE_NAMES
        #undef X
    };

    std::ostream& operator<<(std::ostream&, const Opcode&);

    struct Dynamic_type_value {
        std::variant<std::nullptr_t, bool, double, std::string> variant;
    };

    std::ostream& operator<<(std::ostream&, const Dynamic_type_value&);

    class Chunk {
        std::vector<Dynamic_type_value> constants_;
        std::vector<std::uint8_t> bytecode_;
        std::vector<Token> source_map_tokens_;

        public:
            const auto& constants() const {
                return constants_;
            }

            const auto& bytecode() const {
                return bytecode_;
            }

            const auto& source_map_tokens() const {
                return source_map_tokens_;
            }

            template<Opcode> void emit(const Token& source_map_token);

            void emit_constant(const Dynamic_type_value& constant_value, const Token& source_map_token);
    };

    std::ostream& operator<<(std::ostream&, const Chunk&);

    Chunk compile(std::string_view source);
}}
