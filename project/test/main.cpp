#define BOOST_TEST_MODULE CppLox Test

#include <iterator>
#include <string>

#include <boost/algorithm/string/replace.hpp>
#include <boost/predef.h>
#include <boost/process.hpp>
#include <boost/test/unit_test.hpp>
#include <gsl/span>

using std::istreambuf_iterator;
using std::string;

using boost::process::ipstream;
using boost::process::std_out;
using boost::process::system;
using boost::replace_all;
using boost::unit_test::framework::master_test_suite;
using gsl::span;

const char* main_exe_path() {
    // TODO: Use Program_options here
    static span<char*> argv_span {master_test_suite().argv, master_test_suite().argc};

    return argv_span.at(1);
}

BOOST_AUTO_TEST_CASE(identifiers_test) {
    const auto expected = (
        "IDENTIFIER andy null\n"
        "IDENTIFIER formless null\n"
        "IDENTIFIER fo null\n"
        "IDENTIFIER _ null\n"
        "IDENTIFIER _123 null\n"
        "IDENTIFIER _abc null\n"
        "IDENTIFIER ab123 null\n"
        "IDENTIFIER abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_ null\n"
        "EOF  null\n"
    );

    ipstream cpplox_out;
    const auto exit_code = system(main_exe_path(), "scripts/identifiers.lox", std_out > cpplox_out);
    string actual {istreambuf_iterator<char>{cpplox_out}, istreambuf_iterator<char>{}};

    // Reading from the pipe stream doesn't convert newlines, so we'll have to do that ourselves
    if (BOOST_OS_WINDOWS) {
        replace_all(actual, "\r\n", "\n");
    }

    BOOST_TEST(actual == expected);
    BOOST_TEST(exit_code == 0);
}

BOOST_AUTO_TEST_CASE(keywords_test) {
    const auto expected = (
        "AND and null\n"
        "CLASS class null\n"
        "ELSE else null\n"
        "FALSE false null\n"
        "FOR for null\n"
        "FUN fun null\n"
        "IF if null\n"
        "NIL nil null\n"
        "OR or null\n"
        "RETURN return null\n"
        "SUPER super null\n"
        "THIS this null\n"
        "TRUE true null\n"
        "VAR var null\n"
        "WHILE while null\n"
        "EOF  null\n"
    );

    ipstream cpplox_out;
    const auto exit_code = system(main_exe_path(), "scripts/keywords.lox", std_out > cpplox_out);
    string actual {istreambuf_iterator<char>{cpplox_out}, istreambuf_iterator<char>{}};

    // Reading from the pipe stream doesn't convert newlines, so we'll have to do that ourselves
    if (BOOST_OS_WINDOWS) {
        replace_all(actual, "\r\n", "\n");
    }

    BOOST_TEST(actual == expected);
    BOOST_TEST(exit_code == 0);
}

BOOST_AUTO_TEST_CASE(numbers_test) {
    const auto expected = (
        "NUMBER 123 123.0\n"
        "NUMBER 123.456 123.456\n"
        "DOT . null\n"
        "NUMBER 456 456.0\n"
        "NUMBER 123 123.0\n"
        "DOT . null\n"
        "EOF  null\n"
    );

    ipstream cpplox_out;
    const auto exit_code = system(main_exe_path(), "scripts/numbers.lox", std_out > cpplox_out);
    string actual {istreambuf_iterator<char>{cpplox_out}, istreambuf_iterator<char>{}};

    // Reading from the pipe stream doesn't convert newlines, so we'll have to do that ourselves
    if (BOOST_OS_WINDOWS) {
        replace_all(actual, "\r\n", "\n");
    }

    BOOST_TEST(actual == expected);
    BOOST_TEST(exit_code == 0);
}

BOOST_AUTO_TEST_CASE(punctuators_test) {
    const auto expected = (
        "LEFT_PAREN ( null\n"
        "RIGHT_PAREN ) null\n"
        "LEFT_BRACE { null\n"
        "RIGHT_BRACE } null\n"
        "SEMICOLON ; null\n"
        "COMMA , null\n"
        "PLUS + null\n"
        "MINUS - null\n"
        "STAR * null\n"
        "BANG_EQUAL != null\n"
        "EQUAL_EQUAL == null\n"
        "LESS_EQUAL <= null\n"
        "GREATER_EQUAL >= null\n"
        "BANG_EQUAL != null\n"
        "LESS < null\n"
        "GREATER > null\n"
        "SLASH / null\n"
        "DOT . null\n"
        "EOF  null\n"
    );

    ipstream cpplox_out;
    const auto exit_code = system(main_exe_path(), "scripts/punctuators.lox", std_out > cpplox_out);
    string actual {istreambuf_iterator<char>{cpplox_out}, istreambuf_iterator<char>{}};

    // Reading from the pipe stream doesn't convert newlines, so we'll have to do that ourselves
    if (BOOST_OS_WINDOWS) {
        replace_all(actual, "\r\n", "\n");
    }

    BOOST_TEST(actual == expected);
    BOOST_TEST(exit_code == 0);
}

BOOST_AUTO_TEST_CASE(strings_test) {
    const auto expected = (
        "STRING \"\" \n"
        "STRING \"string\" string\n"
        "EOF  null\n"
    );

    ipstream cpplox_out;
    const auto exit_code = system(main_exe_path(), "scripts/strings.lox", std_out > cpplox_out);
    string actual {istreambuf_iterator<char>{cpplox_out}, istreambuf_iterator<char>{}};

    // Reading from the pipe stream doesn't convert newlines, so we'll have to do that ourselves
    if (BOOST_OS_WINDOWS) {
        replace_all(actual, "\r\n", "\n");
    }

    BOOST_TEST(actual == expected);
    BOOST_TEST(exit_code == 0);
}

BOOST_AUTO_TEST_CASE(whitespace_test) {
    const auto expected = (
        "IDENTIFIER space null\n"
        "IDENTIFIER tabs null\n"
        "IDENTIFIER newlines null\n"
        "IDENTIFIER end null\n"
        "EOF  null\n"
    );

    ipstream cpplox_out;
    const auto exit_code = system(main_exe_path(), "scripts/whitespace.lox", std_out > cpplox_out);
    string actual {istreambuf_iterator<char>{cpplox_out}, istreambuf_iterator<char>{}};

    // Reading from the pipe stream doesn't convert newlines, so we'll have to do that ourselves
    if (BOOST_OS_WINDOWS) {
        replace_all(actual, "\r\n", "\n");
    }

    BOOST_TEST(actual == expected);
    BOOST_TEST(exit_code == 0);
}
