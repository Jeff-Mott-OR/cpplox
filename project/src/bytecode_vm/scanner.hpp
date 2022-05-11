#pragma once

enum class Token_type {
    // Single-character tokens.
    left_paren_, right_paren_,
    left_brace_, right_brace_,
    comma_, dot_, minus_, plus_,
    semicolon_, slash_, star_,

    // One or two character tokens.
    bang_, bang_equal_,
    equal_, equal_equal_,
    greater_, greater_equal_,
    less_, less_equal_,

    // Literals.
    identifier_, string_, number_,

    // Keywords.
    and_, class_, else_, false_,
    for_, fun_, if_, nil_, or_,
    print_, return_, super_, this_,
    true_, var_, while_,

    error_,
    eof_
};

struct Token {
    Token_type type;
    const char* lexeme_begin;
    const char* lexeme_end;
    int line;
};

struct Scanner {
    const char* token_begin;
    const char* token_end;
    const char* source_end;
    int line;

    Scanner(const std::string& source) :
        token_begin {source.data()},
        token_end {source.data()},
        source_end {source.data() + source.size()},
        line {1}
    {
    }
};

Token scanToken(Scanner&);
