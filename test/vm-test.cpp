#define BOOST_TEST_MODULE VM Tests
#include <boost/test/unit_test.hpp>

#include <sstream>
#include <stdexcept>

#include "../src/vm.hpp"

BOOST_AUTO_TEST_CASE(vm_will_run_chunks_of_bytecode) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(motts::lox::Dynamic_type_value{28.0}, motts::lox::Token{motts::lox::Token_type::number, "28", 1});
    chunk.emit_constant(motts::lox::Dynamic_type_value{14.0}, motts::lox::Token{motts::lox::Token_type::number, "14", 1});
    chunk.emit_add(motts::lox::Token{motts::lox::Token_type::plus, "+", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os, /* debug = */ true);

    const auto expected =
        "# Running chunk:\n"
        "Constants:\n"
        "    0 : 28\n"
        "    1 : 14\n"
        "Bytecode:\n"
        "    0 : 00 00 CONSTANT [0] ; 28 @ 1\n"
        "    2 : 00 01 CONSTANT [1] ; 14 @ 1\n"
        "    4 : 04    ADD          ; + @ 1\n"
        "\n"
        "Stack:\n"
        "    0 : 28\n"
        "\n"
        "Stack:\n"
        "    1 : 14\n"
        "    0 : 28\n"
        "\n"
        "Stack:\n"
        "    0 : 42\n"
        "\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(chunks_and_stacks_wont_print_when_debug_is_off) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(motts::lox::Dynamic_type_value{28.0}, motts::lox::Token{motts::lox::Token_type::number, "28", 1});
    chunk.emit_constant(motts::lox::Dynamic_type_value{14.0}, motts::lox::Token{motts::lox::Token_type::number, "14", 1});
    chunk.emit_add(motts::lox::Token{motts::lox::Token_type::plus, "+", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os, /* debug = */ false);

    BOOST_TEST(os.str() == "");
}

BOOST_AUTO_TEST_CASE(numbers_and_strings_add) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(motts::lox::Dynamic_type_value{28.0}, motts::lox::Token{motts::lox::Token_type::number, "28", 1});
    chunk.emit_constant(motts::lox::Dynamic_type_value{14.0}, motts::lox::Token{motts::lox::Token_type::number, "14", 1});
    chunk.emit_add(motts::lox::Token{motts::lox::Token_type::plus, "+", 1});
    chunk.emit_constant(motts::lox::Dynamic_type_value{"hello"}, motts::lox::Token{motts::lox::Token_type::number, "\"hello\"", 1});
    chunk.emit_constant(motts::lox::Dynamic_type_value{"world"}, motts::lox::Token{motts::lox::Token_type::number, "\"world\"", 1});
    chunk.emit_add(motts::lox::Token{motts::lox::Token_type::plus, "+", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os, /* debug = */ true);

    const auto expected =
        "# Running chunk:\n"
        "Constants:\n"
        "    0 : 28\n"
        "    1 : 14\n"
        "    2 : hello\n"
        "    3 : world\n"
        "Bytecode:\n"
        "    0 : 00 00 CONSTANT [0] ; 28 @ 1\n"
        "    2 : 00 01 CONSTANT [1] ; 14 @ 1\n"
        "    4 : 04    ADD          ; + @ 1\n"
        "    5 : 00 02 CONSTANT [2] ; \"hello\" @ 1\n"
        "    7 : 00 03 CONSTANT [3] ; \"world\" @ 1\n"
        "    9 : 04    ADD          ; + @ 1\n"
        "\n"
        "Stack:\n"
        "    0 : 28\n"
        "\n"
        "Stack:\n"
        "    1 : 14\n"
        "    0 : 28\n"
        "\n"
        "Stack:\n"
        "    0 : 42\n"
        "\n"
        "Stack:\n"
        "    1 : hello\n"
        "    0 : 42\n"
        "\n"
        "Stack:\n"
        "    2 : world\n"
        "    1 : hello\n"
        "    0 : 42\n"
        "\n"
        "Stack:\n"
        "    1 : helloworld\n"
        "    0 : 42\n"
        "\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(invalid_plus_will_throw) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(motts::lox::Dynamic_type_value{42.0}, motts::lox::Token{motts::lox::Token_type::number, "42", 1});
    chunk.emit_constant(motts::lox::Dynamic_type_value{"hello"}, motts::lox::Token{motts::lox::Token_type::number, "\"hello\"", 1});
    chunk.emit_add(motts::lox::Token{motts::lox::Token_type::plus, "+", 1});

    BOOST_CHECK_THROW(motts::lox::run(chunk), std::exception);
}

BOOST_AUTO_TEST_CASE(print_whats_on_top_of_stack) {
    motts::lox::Chunk chunk;

    chunk.emit_constant(motts::lox::Dynamic_type_value{42.0}, motts::lox::Token{motts::lox::Token_type::number, "42", 1});
    chunk.emit_print(motts::lox::Token{motts::lox::Token_type::print, "print", 1});

    chunk.emit_constant(motts::lox::Dynamic_type_value{"hello"}, motts::lox::Token{motts::lox::Token_type::string, "hello", 1});
    chunk.emit_print(motts::lox::Token{motts::lox::Token_type::print, "print", 1});

    chunk.emit_constant(motts::lox::Dynamic_type_value{nullptr}, motts::lox::Token{motts::lox::Token_type::nil, "nil", 1});
    chunk.emit_print(motts::lox::Token{motts::lox::Token_type::print, "print", 1});

    chunk.emit_constant(motts::lox::Dynamic_type_value{true}, motts::lox::Token{motts::lox::Token_type::true_, "true", 1});
    chunk.emit_print(motts::lox::Token{motts::lox::Token_type::print, "print", 1});

    chunk.emit_constant(motts::lox::Dynamic_type_value{false}, motts::lox::Token{motts::lox::Token_type::false_, "false", 1});
    chunk.emit_print(motts::lox::Token{motts::lox::Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os);

    const auto expected =
        "42\n"
        "hello\n"
        "nil\n"
        "true\n"
        "false\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(plus_minus_star_slash_will_run) {
    motts::lox::Chunk chunk;

    chunk.emit_constant(motts::lox::Dynamic_type_value{7.0}, motts::lox::Token{motts::lox::Token_type::number, "7", 1});
    chunk.emit_constant(motts::lox::Dynamic_type_value{5.0}, motts::lox::Token{motts::lox::Token_type::number, "5", 1});
    chunk.emit_add(motts::lox::Token{motts::lox::Token_type::plus, "+", 1});

    chunk.emit_constant(motts::lox::Dynamic_type_value{3.0}, motts::lox::Token{motts::lox::Token_type::number, "3", 1});
    chunk.emit_constant(motts::lox::Dynamic_type_value{2.0}, motts::lox::Token{motts::lox::Token_type::number, "2", 1});
    chunk.emit_multiply(motts::lox::Token{motts::lox::Token_type::star, "*", 1});

    chunk.emit_constant(motts::lox::Dynamic_type_value{1.0}, motts::lox::Token{motts::lox::Token_type::number, "1", 1});
    chunk.emit_divide(motts::lox::Token{motts::lox::Token_type::slash, "/", 1});

    chunk.emit_subtract(motts::lox::Token{motts::lox::Token_type::minus, "-", 1});

    chunk.emit_print(motts::lox::Token{motts::lox::Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os);

    BOOST_TEST(os.str() == "6\n");
}

BOOST_AUTO_TEST_CASE(numeric_negation_will_run) {
    motts::lox::Chunk chunk;

    chunk.emit_constant(motts::lox::Dynamic_type_value{1.0}, motts::lox::Token{motts::lox::Token_type::number, "1", 1});
    chunk.emit_negate(motts::lox::Token{motts::lox::Token_type::minus, "-", 1});

    chunk.emit_constant(motts::lox::Dynamic_type_value{1.0}, motts::lox::Token{motts::lox::Token_type::number, "1", 1});
    chunk.emit_negate(motts::lox::Token{motts::lox::Token_type::minus, "-", 1});

    chunk.emit_add(motts::lox::Token{motts::lox::Token_type::plus, "+", 1});

    chunk.emit_print(motts::lox::Token{motts::lox::Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os);

    BOOST_TEST(os.str() == "-2\n");
}

BOOST_AUTO_TEST_CASE(invalid_negation_will_throw) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(motts::lox::Dynamic_type_value{"hello"}, motts::lox::Token{motts::lox::Token_type::string, "\"hello\'", 1});
    chunk.emit_negate(motts::lox::Token{motts::lox::Token_type::minus, "-", 1});
    BOOST_CHECK_THROW(motts::lox::run(chunk), std::exception);
}

BOOST_AUTO_TEST_CASE(false_and_nil_are_falsey_all_else_is_truthy) {
    motts::lox::Chunk chunk;

    chunk.emit_constant(motts::lox::Dynamic_type_value{false}, motts::lox::Token{motts::lox::Token_type::false_, "false", 1});
    chunk.emit_not(motts::lox::Token{motts::lox::Token_type::bang, "!", 1});
    chunk.emit_print(motts::lox::Token{motts::lox::Token_type::print, "print", 1});

    chunk.emit_constant(motts::lox::Dynamic_type_value{nullptr}, motts::lox::Token{motts::lox::Token_type::nil, "nil", 1});
    chunk.emit_not(motts::lox::Token{motts::lox::Token_type::bang, "!", 1});
    chunk.emit_print(motts::lox::Token{motts::lox::Token_type::print, "print", 1});

    chunk.emit_constant(motts::lox::Dynamic_type_value{true}, motts::lox::Token{motts::lox::Token_type::true_, "true", 1});
    chunk.emit_not(motts::lox::Token{motts::lox::Token_type::bang, "!", 1});
    chunk.emit_print(motts::lox::Token{motts::lox::Token_type::print, "print", 1});

    chunk.emit_constant(motts::lox::Dynamic_type_value{1.0}, motts::lox::Token{motts::lox::Token_type::number, "1", 1});
    chunk.emit_not(motts::lox::Token{motts::lox::Token_type::bang, "!", 1});
    chunk.emit_print(motts::lox::Token{motts::lox::Token_type::print, "print", 1});

    chunk.emit_constant(motts::lox::Dynamic_type_value{0.0}, motts::lox::Token{motts::lox::Token_type::number, "0", 1});
    chunk.emit_not(motts::lox::Token{motts::lox::Token_type::bang, "!", 1});
    chunk.emit_print(motts::lox::Token{motts::lox::Token_type::print, "print", 1});

    chunk.emit_constant(motts::lox::Dynamic_type_value{"hello"}, motts::lox::Token{motts::lox::Token_type::string, "\"hello\"", 1});
    chunk.emit_not(motts::lox::Token{motts::lox::Token_type::bang, "!", 1});
    chunk.emit_print(motts::lox::Token{motts::lox::Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os);

    BOOST_TEST(os.str() == "true\ntrue\nfalse\nfalse\nfalse\nfalse\n");
}

BOOST_AUTO_TEST_CASE(pop_will_run) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(motts::lox::Dynamic_type_value{42.0}, motts::lox::Token{motts::lox::Token_type::number, "42", 1});
    chunk.emit_pop(motts::lox::Token{motts::lox::Token_type::semicolon, ";", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os, /* debug = */ true);

    const auto expected =
        "# Running chunk:\n"
        "Constants:\n"
        "    0 : 42\n"
        "Bytecode:\n"
        "    0 : 00 00 CONSTANT [0] ; 42 @ 1\n"
        "    2 : 0b    POP          ; ; @ 1\n"
        "\n"
        "Stack:\n"
        "    0 : 42\n"
        "\n"
        "Stack:\n"
        "\n";
    BOOST_TEST(os.str() == expected);
}
