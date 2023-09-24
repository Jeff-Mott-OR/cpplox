#define BOOST_TEST_MODULE Compiler Tests
#include <boost/test/unit_test.hpp>

#include <sstream>
#include <stdexcept>

#include "../src/compiler.hpp"

BOOST_AUTO_TEST_CASE(opcodes_can_be_printed) {
    std::ostringstream os;
    os << motts::lox::Opcode::constant;
    BOOST_TEST(os.str() == "CONSTANT");
}

BOOST_AUTO_TEST_CASE(printing_invalid_opcode_will_throw) {
    const auto invalid_opcode_value = 276'709; // don't panic
    const auto invalid_opcode = reinterpret_cast<const motts::lox::Opcode&>(invalid_opcode_value);

    std::ostringstream os;
    BOOST_CHECK_THROW(os << invalid_opcode, std::exception);
}

BOOST_AUTO_TEST_CASE(chunks_can_be_printed) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(motts::lox::Dynamic_type_value{42.0}, motts::lox::Token{motts::lox::Token_type::number, "42", 1});
    chunk.emit_nil(motts::lox::Token{motts::lox::Token_type::nil, "nil", 2});

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 42\n"
        "Bytecode:\n"
        "    0 : 00 00 CONSTANT [0] ; 42 @ 1\n"
        "    2 : 01    NIL          ; nil @ 2\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(number_literals_compile) {
    const auto chunk = motts::lox::compile("42;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 42\n"
        "Bytecode:\n"
        "    0 : 00 00 CONSTANT [0] ; 42 @ 1\n"
        "    2 : 0b    POP          ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(nil_literals_compile) {
    const auto chunk = motts::lox::compile("nil;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 01    NIL          ; nil @ 1\n"
        "    1 : 0b    POP          ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(invalid_expressions_will_throw) {
    BOOST_CHECK_THROW(motts::lox::compile("?"), std::exception);
}

BOOST_AUTO_TEST_CASE(true_false_literals_compile) {
    const auto chunk = motts::lox::compile("true; false;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 02    TRUE         ; true @ 1\n"
        "    1 : 0b    POP          ; ; @ 1\n"
        "    2 : 03    FALSE        ; false @ 1\n"
        "    3 : 0b    POP          ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(string_literals_compile) {
    const auto chunk = motts::lox::compile("\"hello\";");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : hello\n"
        "Bytecode:\n"
        "    0 : 00 00 CONSTANT [0] ; \"hello\" @ 1\n"
        "    2 : 0b    POP          ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(addition_will_compile) {
    const auto chunk = motts::lox::compile("28 + 14;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 28\n"
        "    1 : 14\n"
        "Bytecode:\n"
        "    0 : 00 00 CONSTANT [0] ; 28 @ 1\n"
        "    2 : 00 01 CONSTANT [1] ; 14 @ 1\n"
        "    4 : 04    ADD          ; + @ 1\n"
        "    5 : 0b    POP          ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(invalid_addition_will_throw) {
    BOOST_CHECK_THROW(motts::lox::compile("42 + "), std::exception);
}

BOOST_AUTO_TEST_CASE(print_will_compile) {
    const auto chunk = motts::lox::compile("print 42;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 42\n"
        "Bytecode:\n"
        "    0 : 00 00 CONSTANT [0] ; 42 @ 1\n"
        "    2 : 05    PRINT        ; print @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(plus_minus_star_slash_will_compile) {
    const auto chunk = motts::lox::compile("1 + 2 - 3 * 5 / 7;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 1\n"
        "    1 : 2\n"
        "    2 : 3\n"
        "    3 : 5\n"
        "    4 : 7\n"
        "Bytecode:\n"
        "    0 : 00 00 CONSTANT [0] ; 1 @ 1\n"
        "    2 : 00 01 CONSTANT [1] ; 2 @ 1\n"
        "    4 : 04    ADD          ; + @ 1\n"
        "    5 : 00 02 CONSTANT [2] ; 3 @ 1\n"
        "    7 : 00 03 CONSTANT [3] ; 5 @ 1\n"
        "    9 : 07    MULTIPLY     ; * @ 1\n"
        "   10 : 00 04 CONSTANT [4] ; 7 @ 1\n"
        "   12 : 08    DIVIDE       ; / @ 1\n"
        "   13 : 06    SUBTRACT     ; - @ 1\n"
        "   14 : 0b    POP          ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(parens_will_compile) {
    const auto chunk = motts::lox::compile("1 + (2 - 3) * 5 / 7;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 1\n"
        "    1 : 2\n"
        "    2 : 3\n"
        "    3 : 5\n"
        "    4 : 7\n"
        "Bytecode:\n"
        "    0 : 00 00 CONSTANT [0] ; 1 @ 1\n"
        "    2 : 00 01 CONSTANT [1] ; 2 @ 1\n"
        "    4 : 00 02 CONSTANT [2] ; 3 @ 1\n"
        "    6 : 06    SUBTRACT     ; - @ 1\n"
        "    7 : 00 03 CONSTANT [3] ; 5 @ 1\n"
        "    9 : 07    MULTIPLY     ; * @ 1\n"
        "   10 : 00 04 CONSTANT [4] ; 7 @ 1\n"
        "   12 : 08    DIVIDE       ; / @ 1\n"
        "   13 : 04    ADD          ; + @ 1\n"
        "   14 : 0b    POP          ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(numeric_negation_will_compile) {
    const auto chunk = motts::lox::compile("-1 + -1;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 1\n"
        "    1 : 1\n"
        "Bytecode:\n"
        "    0 : 00 00 CONSTANT [0] ; 1 @ 1\n"
        "    2 : 09    NEGATE       ; - @ 1\n"
        "    3 : 00 01 CONSTANT [1] ; 1 @ 1\n"
        "    5 : 09    NEGATE       ; - @ 1\n"
        "    6 : 04    ADD          ; + @ 1\n"
        "    7 : 0b    POP          ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(boolean_negation_will_compile) {
    const auto chunk = motts::lox::compile("!true;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 02    TRUE         ; true @ 1\n"
        "    1 : 0a    NOT          ; ! @ 1\n"
        "    2 : 0b    POP          ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(expression_statements_will_compile) {
    const auto chunk = motts::lox::compile("42;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 42\n"
        "Bytecode:\n"
        "    0 : 00 00 CONSTANT [0] ; 42 @ 1\n"
        "    2 : 0b    POP          ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}
