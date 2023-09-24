#define BOOST_TEST_MODULE Scanner Tests
#include <boost/test/unit_test.hpp>

#include <sstream>
#include <stdexcept>

#include "../src/scanner.hpp"

BOOST_AUTO_TEST_CASE(token_types_can_be_printed) {
    {
        std::ostringstream os;
        os << motts::lox::Token_type::number;
        BOOST_TEST(os.str() == "NUMBER");
    }
    {
        std::ostringstream os;
        os << motts::lox::Token_type::and_;
        BOOST_TEST(os.str() == "AND");
    }
}

BOOST_AUTO_TEST_CASE(printing_invalid_token_types_will_throw) {
    union Invalid_token_type {
        motts::lox::Token_type as_token_type;
        int as_int;
    };
    Invalid_token_type invalid_token_type;
    invalid_token_type.as_int = 276'709; // don't panic

    std::ostringstream os;
    BOOST_CHECK_THROW(os << invalid_token_type.as_token_type, std::exception);
}

BOOST_AUTO_TEST_CASE(single_character_punctuation_tokenize) {
    motts::lox::Token_iterator token_iter {"(){},.-+;/*"};
    motts::lox::Token_iterator token_iter_end;

    std::ostringstream os;
    for (; token_iter != token_iter_end; ++token_iter) {
        os << *token_iter << "\n";
    }

    const auto expected =
        "Token { type: LEFT_PAREN, lexeme: (, line: 1 }\n"
        "Token { type: RIGHT_PAREN, lexeme: ), line: 1 }\n"
        "Token { type: LEFT_BRACE, lexeme: {, line: 1 }\n"
        "Token { type: RIGHT_BRACE, lexeme: }, line: 1 }\n"
        "Token { type: COMMA, lexeme: ,, line: 1 }\n"
        "Token { type: DOT, lexeme: ., line: 1 }\n"
        "Token { type: MINUS, lexeme: -, line: 1 }\n"
        "Token { type: PLUS, lexeme: +, line: 1 }\n"
        "Token { type: SEMICOLON, lexeme: ;, line: 1 }\n"
        "Token { type: SLASH, lexeme: /, line: 1 }\n"
        "Token { type: STAR, lexeme: *, line: 1 }\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(tokens_track_what_line_they_came_from) {
    motts::lox::Token_iterator token_iter {"one\ntwo\nthree\n"};
    motts::lox::Token_iterator token_iter_end;

    std::ostringstream os;
    for (; token_iter != token_iter_end; ++token_iter) {
        os << *token_iter << "\n";
    }

    const auto expected =
        "Token { type: IDENTIFIER, lexeme: one, line: 1 }\n"
        "Token { type: IDENTIFIER, lexeme: two, line: 2 }\n"
        "Token { type: IDENTIFIER, lexeme: three, line: 3 }\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(two_consecutive_slashes_means_line_comment) {
    motts::lox::Token_iterator token_iter {"// line comment\nnext line\n"};
    motts::lox::Token_iterator token_iter_end;

    std::ostringstream os;
    for (; token_iter != token_iter_end; ++token_iter) {
        os << *token_iter << "\n";
    }

    const auto expected =
        "Token { type: IDENTIFIER, lexeme: next, line: 2 }\n"
        "Token { type: IDENTIFIER, lexeme: line, line: 2 }\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(whitespace_is_skipped_and_ignored) {
    motts::lox::Token_iterator token_iter {"one\r\n\t two"};
    motts::lox::Token_iterator token_iter_end;

    std::ostringstream os;
    for (; token_iter != token_iter_end; ++token_iter) {
        os << *token_iter << "\n";
    }

    const auto expected =
        "Token { type: IDENTIFIER, lexeme: one, line: 1 }\n"
        "Token { type: IDENTIFIER, lexeme: two, line: 2 }\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(double_quoted_strings_tokenize) {
    motts::lox::Token_iterator token_iter {"\"123 and ( { ,\""};
    motts::lox::Token_iterator token_iter_end;

    std::ostringstream os;
    for (; token_iter != token_iter_end; ++token_iter) {
        os << *token_iter << "\n";
    }

    BOOST_TEST(os.str() == "Token { type: STRING, lexeme: \"123 and ( { ,\", line: 1 }\n");
}

BOOST_AUTO_TEST_CASE(strings_can_be_multiline) {
    motts::lox::Token_iterator token_iter {"\"123\nand\""};
    motts::lox::Token_iterator token_iter_end;

    std::ostringstream os;
    for (; token_iter != token_iter_end; ++token_iter) {
        os << *token_iter << "\n";
    }

    BOOST_TEST(os.str() == "Token { type: STRING, lexeme: \"123\nand\", line: 1 }\n");
}

BOOST_AUTO_TEST_CASE(unterminated_strings_will_throw) {
    BOOST_CHECK_THROW(motts::lox::Token_iterator{"\""}, std::exception);
}

BOOST_AUTO_TEST_CASE(some_identifiers_will_be_keywords) {
    motts::lox::Token_iterator token_iter {
        "and class else false for fun if nil or print return super this true var while break continue"
    };
    motts::lox::Token_iterator token_iter_end;

    std::ostringstream os;
    for (; token_iter != token_iter_end; ++token_iter) {
        os << *token_iter << "\n";
    }

    const auto expected =
        "Token { type: AND, lexeme: and, line: 1 }\n"
        "Token { type: CLASS, lexeme: class, line: 1 }\n"
        "Token { type: ELSE, lexeme: else, line: 1 }\n"
        "Token { type: FALSE, lexeme: false, line: 1 }\n"
        "Token { type: FOR, lexeme: for, line: 1 }\n"
        "Token { type: FUN, lexeme: fun, line: 1 }\n"
        "Token { type: IF, lexeme: if, line: 1 }\n"
        "Token { type: NIL, lexeme: nil, line: 1 }\n"
        "Token { type: OR, lexeme: or, line: 1 }\n"
        "Token { type: PRINT, lexeme: print, line: 1 }\n"
        "Token { type: RETURN, lexeme: return, line: 1 }\n"
        "Token { type: SUPER, lexeme: super, line: 1 }\n"
        "Token { type: THIS, lexeme: this, line: 1 }\n"
        "Token { type: TRUE, lexeme: true, line: 1 }\n"
        "Token { type: VAR, lexeme: var, line: 1 }\n"
        "Token { type: WHILE, lexeme: while, line: 1 }\n"
        "Token { type: BREAK, lexeme: break, line: 1 }\n"
        "Token { type: CONTINUE, lexeme: continue, line: 1 }\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(multi_character_punctuation_tokenize) {
    motts::lox::Token_iterator token_iter {"! != = == > >= < <="};
    motts::lox::Token_iterator token_iter_end;

    std::ostringstream os;
    for (; token_iter != token_iter_end; ++token_iter) {
        os << *token_iter << "\n";
    }

    const auto expected =
        "Token { type: BANG, lexeme: !, line: 1 }\n"
        "Token { type: BANG_EQUAL, lexeme: !=, line: 1 }\n"
        "Token { type: EQUAL, lexeme: =, line: 1 }\n"
        "Token { type: EQUAL_EQUAL, lexeme: ==, line: 1 }\n"
        "Token { type: GREATER, lexeme: >, line: 1 }\n"
        "Token { type: GREATER_EQUAL, lexeme: >=, line: 1 }\n"
        "Token { type: LESS, lexeme: <, line: 1 }\n"
        "Token { type: LESS_EQUAL, lexeme: <=, line: 1 }\n";
    BOOST_TEST(os.str() == expected);

}
