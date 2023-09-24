#define BOOST_TEST_MODULE CppLox Tests
#include <boost/test/unit_test.hpp>

#include <sstream>
#include <stdexcept>

#include "scanner.hpp"

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
    std::ostringstream os;
    const auto invalid_token_type_value = 276'709; // don't panic
    const auto invalid_token_type = reinterpret_cast<const motts::lox::Token_type&>(invalid_token_type_value);
    BOOST_CHECK_THROW(os << invalid_token_type, std::exception);
}

BOOST_AUTO_TEST_CASE(single_characters_tokenize) {
    std::ostringstream os;
    motts::lox::Token_iterator token_iter {"(){},.-+;/*"};
    motts::lox::Token_iterator token_iter_end;
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
    std::ostringstream os;
    motts::lox::Token_iterator token_iter {"one\ntwo\nthree\n"};
    motts::lox::Token_iterator token_iter_end;
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
    std::ostringstream os;
    motts::lox::Token_iterator token_iter {"// line comment\nnext line\n"};
    motts::lox::Token_iterator token_iter_end;
    for (; token_iter != token_iter_end; ++token_iter) {
        os << *token_iter << "\n";
    }

    const auto expected =
        "Token { type: IDENTIFIER, lexeme: next, line: 2 }\n"
        "Token { type: IDENTIFIER, lexeme: line, line: 2 }\n";
    BOOST_TEST(os.str() == expected);
}
