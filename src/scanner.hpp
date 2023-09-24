#pragma once

#include <ostream>
#include <string_view>

namespace motts { namespace lox {
    #define MOTTS_LOX_TOKEN_TYPE_NAMES \
        /* Single-character tokens */ \
        X(left_paren) X(right_paren) \
        X(left_brace) X(right_brace) \
        X(comma) X(dot) X(minus) X(plus) \
        X(semicolon) X(slash) X(star) \
        \
        /* Literals */ \
        X(identifier) X(string) X(number) \
        \
        /* Keywords */ \
        X(and_) \
        X(break_) \
        X(class_) \
        X(continue_) \
        X(else_) \
        X(false_) \
        X(for_) \
        X(fun) \
        X(if_) \
        X(nil) \
        X(or_) \
        X(print) \
        X(return_) \
        X(super) \
        X(this_) \
        X(true_) \
        X(var) \
        X(while_) \
        \
        X(eof)

    enum class Token_type {
        #define X(name) name,
        MOTTS_LOX_TOKEN_TYPE_NAMES
        #undef X
    };

    std::ostream& operator<<(std::ostream&, const Token_type&);

    struct Token {
        Token_type type;
        std::string_view lexeme;
        int line;
    };

    std::ostream& operator<<(std::ostream&, const Token&);

    class Token_iterator {
        std::string_view::const_iterator token_begin_;
        std::string_view::const_iterator token_end_;
        std::string_view::const_iterator source_end_;
        int line_ {1};
        Token token_;

        public:
            Token_iterator(std::string_view source);

            // Default constructor makes end iterator
            Token_iterator();

            Token_iterator& operator++();
            bool operator==(const Token_iterator&) const;

            const Token& operator*() const;
            const Token* operator->() const;

        private:
            Token scan_token();
            Token scan_identifier_token();
            Token scan_number_token();
            Token scan_string_token();
            bool scan_if_match(char expected);
    };

    bool operator!=(const Token_iterator&, const Token_iterator&);

}}
