#pragma once

#include <iterator>
#include <ostream>
#include <stdexcept>
#include <string>

namespace motts { namespace lox {
    // X-macro
    #define MOTTS_LOX_TOKEN_TYPE_NAMES \
        X(left_paren) X(right_paren) \
        X(left_brace) X(right_brace) \
        X(comma) X(dot) X(minus) X(plus) \
        X(semicolon) X(slash) X(star) \
        \
        X(bang) X(bang_equal) \
        X(equal) X(equal_equal) \
        X(greater) X(greater_equal) \
        X(less) X(less_equal) \
        \
        X(identifier) X(string) X(number) \
        \
        X(and_) X(class_) X(else_) X(false_) \
        X(for_) X(fun) X(if_) X(nil) X(or_) \
        X(print) X(return_) X(super) X(this_) \
        X(true_) X(var) X(while_) \
        \
        X(eof)

    enum class Token_type {
        #define X(name) name,
        MOTTS_LOX_TOKEN_TYPE_NAMES
        #undef X
    };

    std::ostream& operator<<(std::ostream&, Token_type);

    struct Token {
        Token_type type;
        std::string::const_iterator begin;
        std::string::const_iterator end;
        int line;
    };

    bool operator==(const Token&, const Token&);

    class Token_iterator : public std::iterator<std::forward_iterator_tag, Token> {
        public:
            // Begin
            explicit Token_iterator(const std::string& source);

            // End
            explicit Token_iterator();

            Token_iterator& operator++();
            Token_iterator operator++(int);

            bool operator==(const Token_iterator&) const;
            bool operator!=(const Token_iterator&) const;

            const Token& operator*() const &;
            Token&& operator*() &&;

            const Token* operator->() const;

        private:
            Token consume_token();
            void consume_whitespace();
            Token consume_identifier();
            Token consume_number();
            Token consume_string();

            Token make_token(Token_type);
            bool advance_if_match(char expected);
            Token_type identifier_type();
            Token_type keyword_or_identifier(
                std::string::const_iterator begin,
                const char* rest_keyword,
                Token_type type_if_match
            );

            std::string::const_iterator token_begin_;
            std::string::const_iterator token_end_;
            std::string::const_iterator source_end_;
            Token token_;
            int line_ {1};
    };

    struct Scanner_error : std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}}
