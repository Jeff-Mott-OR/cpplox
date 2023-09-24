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
    chunk.emit_constant(42, motts::lox::Token{motts::lox::Token_type::number, "42", 1});
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
    const auto chunk = motts::lox::compile("42");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 42\n"
        "Bytecode:\n"
        "    0 : 00 00 CONSTANT [0] ; 42 @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(nil_literals_compile) {
    const auto chunk = motts::lox::compile("nil");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 01    NIL          ; nil @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(invalid_expressions_will_throw) {
    BOOST_CHECK_THROW(motts::lox::compile("?"), std::exception);
}

BOOST_AUTO_TEST_CASE(true_false_literals_compile) {
    const auto chunk = motts::lox::compile("true false");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 02    TRUE         ; true @ 1\n"
        "    1 : 03    FALSE        ; false @ 1\n";
    BOOST_TEST(os.str() == expected);
}
