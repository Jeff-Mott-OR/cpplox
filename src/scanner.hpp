#pragma once

#include <ostream>
#include <string_view>

namespace motts::lox
{
// X-macro technique to re-use this list in multiple places.
#define MOTTS_LOX_TOKEN_TYPE_NAMES \
    X(eof) \
\
    /* Single-character tokens */ \
    X(left_paren) \
    X(right_paren) \
    X(left_brace) \
    X(right_brace) \
    X(comma) \
    X(dot) \
    X(minus) \
    X(plus) \
    X(semicolon) \
    X(slash) \
    X(star) \
\
    /* One or two character tokens */ \
    X(bang) \
    X(bang_equal) \
    X(equal) \
    X(equal_equal) \
    X(greater) \
    X(greater_equal) \
    X(less) \
    X(less_equal) \
\
    /* Literals */ \
    X(identifier) \
    X(number) \
    X(string) \
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
    X(while_)

    enum class Token_type
    {
#define X(name) name,
        MOTTS_LOX_TOKEN_TYPE_NAMES
#undef X
    };

    std::ostream& operator<<(std::ostream&, Token_type);

    struct Token
    {
        Token_type type;
        std::string_view lexeme;
        unsigned int line;
    };

    std::ostream& operator<<(std::ostream&, const Token&);

    class Token_iterator
    {
        std::string_view::const_iterator token_begin_{};
        std::string_view::const_iterator token_end_{};
        std::string_view::const_iterator source_end_{};
        unsigned int line_{1};

        // It's important that `token_` be last because its initialization depends on the string iterators.
        Token token_;

      public:
        Token_iterator(std::string_view source);

        // Default constructor makes end iterator.
        Token_iterator();

        Token_iterator& operator++();
        Token_iterator operator++(int);

        const Token& operator*() const;
        const Token* operator->() const;

        bool operator==(const Token_iterator&) const;

      private:
        Token scan_token();
        Token scan_identifier_token();
        Token scan_number_token();
        Token scan_string_token();

        bool scan_if_match(char expected);
    };

    bool operator!=(const Token_iterator&, const Token_iterator&);
}
