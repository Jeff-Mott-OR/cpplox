#pragma once

#include <gsl/string_span>

namespace motts { namespace lox {
    enum class Token_type {
        // Single-character tokens
        left_paren, right_paren,
        left_brace, right_brace,
        comma, dot, minus, plus,
        semicolon, slash, star,

        // One or two character tokens
        bang, bang_equal,
        equal, equal_equal,
        greater, greater_equal,
        less, less_equal,

        // Literals
        identifier, string, number,

        // Keywords
        and_, class_, else_, false_,
        for_, fun_, if_, nil, or_,
        print, return_, super_, this_,
        true_, var_, while_,
        break_, continue_,

        error,
        eof
    };

    // Cheap to copy; two ints and two pointers
    struct Token {
        Token_type type;
        gsl::cstring_span<> lexeme;
        int line {};
    };

    class Token_iterator {
        const char* token_begin_;
        const char* token_end_;
        const char* source_end_;
        int line_ {1};
        Token token_;

        public:
            Token_iterator(const gsl::cstring_span<>& source);

            const Token& operator*() const;
            const Token* operator->() const;

            Token_iterator& operator++();
            Token_iterator operator++(int);

        private:
            Token consume_token();
            Token consume_identifier();
            Token consume_number();
            Token consume_string();
            bool consume_if_match(char expected);
    };
}}
