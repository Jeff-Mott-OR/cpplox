#pragma once

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
        X(false_)

    enum class Opcode {
        #define X(name) name,
        MOTTS_LOX_OPCODE_NAMES
        #undef X
    };

    std::ostream& operator<<(std::ostream&, const Opcode&);

    struct Dynamic_type_value {
        std::variant<double, std::string> variant;
    };

    std::ostream& operator<<(std::ostream&, const Dynamic_type_value&);

    class Chunk {
        std::vector<Dynamic_type_value> constants_;
        std::vector<std::uint8_t> bytecode_;
        std::vector<Token> source_map_tokens_;

        public:
            const decltype(constants_)& constants() const;
            const decltype(bytecode_)& bytecode() const;
            const decltype(source_map_tokens_)& source_map_tokens() const;

            void emit_constant(const Dynamic_type_value& constant_value, const Token& source_map_token);
            void emit_false(const Token& source_map_token);
            void emit_nil(const Token& source_map_token);
            void emit_true(const Token& source_map_token);
    };

    std::ostream& operator<<(std::ostream&, const Chunk&);

    Chunk compile(std::string_view source);
}}
