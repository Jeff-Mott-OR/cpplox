#define BOOST_TEST_MODULE VM Tests
#include <boost/test/unit_test.hpp>

#include <sstream>
#include <stdexcept>

#include "../src/vm.hpp"

using motts::lox::Dynamic_type_value;
using motts::lox::Opcode;
using motts::lox::Token;
using motts::lox::Token_type;

BOOST_AUTO_TEST_CASE(vm_will_run_chunks_of_bytecode) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(Dynamic_type_value{28.0}, Token{Token_type::number, "28", 1});
    chunk.emit_constant(Dynamic_type_value{14.0}, Token{Token_type::number, "14", 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os, /* debug = */ true);

    const auto expected =
        "# Running chunk:\n"
        "Constants:\n"
        "    0 : 28\n"
        "    1 : 14\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 28 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 14 @ 1\n"
        "    4 : 04       ADD                     ; + @ 1\n"
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
    chunk.emit_constant(Dynamic_type_value{28.0}, Token{Token_type::number, "28", 1});
    chunk.emit_constant(Dynamic_type_value{14.0}, Token{Token_type::number, "14", 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os, /* debug = */ false);

    BOOST_TEST(os.str() == "");
}

BOOST_AUTO_TEST_CASE(numbers_and_strings_add) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(Dynamic_type_value{28.0}, Token{Token_type::number, "28", 1});
    chunk.emit_constant(Dynamic_type_value{14.0}, Token{Token_type::number, "14", 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});
    chunk.emit_constant(Dynamic_type_value{"hello"}, Token{Token_type::number, "\"hello\"", 1});
    chunk.emit_constant(Dynamic_type_value{"world"}, Token{Token_type::number, "\"world\"", 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});

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
        "    0 : 00 00    CONSTANT [0]            ; 28 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 14 @ 1\n"
        "    4 : 04       ADD                     ; + @ 1\n"
        "    5 : 00 02    CONSTANT [2]            ; \"hello\" @ 1\n"
        "    7 : 00 03    CONSTANT [3]            ; \"world\" @ 1\n"
        "    9 : 04       ADD                     ; + @ 1\n"
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
    chunk.emit_constant(Dynamic_type_value{42.0}, Token{Token_type::number, "42", 1});
    chunk.emit_constant(Dynamic_type_value{"hello"}, Token{Token_type::number, "\"hello\"", 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});

    BOOST_CHECK_THROW(motts::lox::run(chunk), std::exception);
    try {
        motts::lox::run(chunk);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error: Operands must be two numbers or two strings.");
    }
}

BOOST_AUTO_TEST_CASE(print_whats_on_top_of_stack) {
    motts::lox::Chunk chunk;

    chunk.emit_constant(Dynamic_type_value{42.0}, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{"hello"}, Token{Token_type::string, "hello", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{nullptr}, Token{Token_type::nil, "nil", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{true}, Token{Token_type::true_, "true", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{false}, Token{Token_type::false_, "false", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

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

    chunk.emit_constant(Dynamic_type_value{7.0}, Token{Token_type::number, "7", 1});
    chunk.emit_constant(Dynamic_type_value{5.0}, Token{Token_type::number, "5", 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});

    chunk.emit_constant(Dynamic_type_value{3.0}, Token{Token_type::number, "3", 1});
    chunk.emit_constant(Dynamic_type_value{2.0}, Token{Token_type::number, "2", 1});
    chunk.emit<Opcode::multiply>(Token{Token_type::star, "*", 1});

    chunk.emit_constant(Dynamic_type_value{1.0}, Token{Token_type::number, "1", 1});
    chunk.emit<Opcode::divide>(Token{Token_type::slash, "/", 1});

    chunk.emit<Opcode::subtract>(Token{Token_type::minus, "-", 1});

    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os);

    BOOST_TEST(os.str() == "6\n");
}

BOOST_AUTO_TEST_CASE(invalid_plus_minus_star_slash_will_throw) {
    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(Dynamic_type_value{42.0}, Token{Token_type::number, "42", 1});
        chunk.emit_constant(Dynamic_type_value{true}, Token{Token_type::true_, "true", 1});
        chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});
        BOOST_CHECK_THROW(motts::lox::run(chunk), std::exception);
        try {
            motts::lox::run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error: Operands must be two numbers or two strings.");
        }
    }
    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(Dynamic_type_value{42.0}, Token{Token_type::number, "42", 1});
        chunk.emit_constant(Dynamic_type_value{true}, Token{Token_type::true_, "true", 1});
        chunk.emit<Opcode::subtract>(Token{Token_type::minus, "-", 1});
        BOOST_CHECK_THROW(motts::lox::run(chunk), std::exception);
        try {
            motts::lox::run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error: Operands must be numbers.");
        }
    }
    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(Dynamic_type_value{42.0}, Token{Token_type::number, "42", 1});
        chunk.emit_constant(Dynamic_type_value{true}, Token{Token_type::true_, "true", 1});
        chunk.emit<Opcode::multiply>(Token{Token_type::star, "*", 1});
        BOOST_CHECK_THROW(motts::lox::run(chunk), std::exception);
        try {
            motts::lox::run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error: Operands must be numbers.");
        }
    }
    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(Dynamic_type_value{42.0}, Token{Token_type::number, "42", 1});
        chunk.emit_constant(Dynamic_type_value{true}, Token{Token_type::true_, "true", 1});
        chunk.emit<Opcode::divide>(Token{Token_type::slash, "/", 1});
        BOOST_CHECK_THROW(motts::lox::run(chunk), std::exception);
        try {
            motts::lox::run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error: Operands must be numbers.");
        }
    }
}

BOOST_AUTO_TEST_CASE(numeric_negation_will_run) {
    motts::lox::Chunk chunk;

    chunk.emit_constant(Dynamic_type_value{1.0}, Token{Token_type::number, "1", 1});
    chunk.emit<Opcode::negate>(Token{Token_type::minus, "-", 1});

    chunk.emit_constant(Dynamic_type_value{1.0}, Token{Token_type::number, "1", 1});
    chunk.emit<Opcode::negate>(Token{Token_type::minus, "-", 1});

    chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});

    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os);

    BOOST_TEST(os.str() == "-2\n");
}

BOOST_AUTO_TEST_CASE(invalid_negation_will_throw) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(Dynamic_type_value{"hello"}, Token{Token_type::string, "\"hello\'", 1});
    chunk.emit<Opcode::negate>(Token{Token_type::minus, "-", 1});
    BOOST_CHECK_THROW(motts::lox::run(chunk), std::exception);
    try {
        motts::lox::run(chunk);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error: Operand must be a number.");
    }
}

BOOST_AUTO_TEST_CASE(false_and_nil_are_falsey_all_else_is_truthy) {
    motts::lox::Chunk chunk;

    chunk.emit_constant(Dynamic_type_value{false}, Token{Token_type::false_, "false", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, "!", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{nullptr}, Token{Token_type::nil, "nil", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, "!", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{true}, Token{Token_type::true_, "true", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, "!", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{1.0}, Token{Token_type::number, "1", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, "!", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{0.0}, Token{Token_type::number, "0", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, "!", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{"hello"}, Token{Token_type::string, "\"hello\"", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, "!", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os);

    BOOST_TEST(os.str() == "true\ntrue\nfalse\nfalse\nfalse\nfalse\n");
}

BOOST_AUTO_TEST_CASE(pop_will_run) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(Dynamic_type_value{42.0}, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::pop>(Token{Token_type::semicolon, ";", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os, /* debug = */ true);

    const auto expected =
        "# Running chunk:\n"
        "Constants:\n"
        "    0 : 42\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 0b       POP                     ; ; @ 1\n"
        "\n"
        "Stack:\n"
        "    0 : 42\n"
        "\n"
        "Stack:\n"
        "\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(comparisons_will_run) {
    motts::lox::Chunk chunk;

    chunk.emit_constant(Dynamic_type_value{1.0}, Token{Token_type::number, "1", 1});
    chunk.emit_constant(Dynamic_type_value{2.0}, Token{Token_type::number, "2", 1});
    chunk.emit<Opcode::greater>(Token{Token_type::greater, ">", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{3.0}, Token{Token_type::number, "3", 1});
    chunk.emit_constant(Dynamic_type_value{5.0}, Token{Token_type::number, "5", 1});
    chunk.emit<Opcode::less>(Token{Token_type::greater_equal, ">=", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::greater_equal, ">=", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{7.0}, Token{Token_type::number, "7", 1});
    chunk.emit_constant(Dynamic_type_value{11.0}, Token{Token_type::number, "11", 1});
    chunk.emit<Opcode::equal>(Token{Token_type::equal_equal, "==", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{13.0}, Token{Token_type::number, "13", 1});
    chunk.emit_constant(Dynamic_type_value{17.0}, Token{Token_type::number, "17", 1});
    chunk.emit<Opcode::equal>(Token{Token_type::bang_equal, "!=", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang_equal, "!=", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{19.0}, Token{Token_type::number, "19", 1});
    chunk.emit_constant(Dynamic_type_value{23.0}, Token{Token_type::number, "23", 1});
    chunk.emit<Opcode::greater>(Token{Token_type::less_equal, "<=", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::less_equal, "<=", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{29.0}, Token{Token_type::number, "29", 1});
    chunk.emit_constant(Dynamic_type_value{31.0}, Token{Token_type::number, "31", 1});
    chunk.emit<Opcode::less>(Token{Token_type::less, "<", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{"42"}, Token{Token_type::string, "\"42\"", 1});
    chunk.emit_constant(Dynamic_type_value{"42"}, Token{Token_type::string, "\"42\"", 1});
    chunk.emit<Opcode::equal>(Token{Token_type::equal_equal, "==", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(Dynamic_type_value{"42"}, Token{Token_type::string, "\"42\"", 1});
    chunk.emit_constant(Dynamic_type_value{42.0}, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::equal>(Token{Token_type::equal_equal, "==", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os);

    BOOST_TEST(os.str() == "false\nfalse\nfalse\ntrue\ntrue\ntrue\ntrue\nfalse\n");
}

BOOST_AUTO_TEST_CASE(invalid_comparisons_will_throw) {
    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(Dynamic_type_value{1.0}, Token{Token_type::number, "1", 1});
        chunk.emit_constant(Dynamic_type_value{true}, Token{Token_type::true_, "true", 1});
        chunk.emit<Opcode::greater>(Token{Token_type::greater, ">", 1});
        BOOST_CHECK_THROW(motts::lox::run(chunk), std::exception);
        try {
            motts::lox::run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error: Operands must be numbers.");
        }
    }
    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(Dynamic_type_value{"hello"}, Token{Token_type::string, "\"hello\"", 1});
        chunk.emit_constant(Dynamic_type_value{1.0}, Token{Token_type::number, "1", 1});
        chunk.emit<Opcode::less>(Token{Token_type::less, "<", 1});
        BOOST_CHECK_THROW(motts::lox::run(chunk), std::exception);
        try {
            motts::lox::run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error: Operands must be numbers.");
        }
    }
}

BOOST_AUTO_TEST_CASE(jump_if_false_will_run) {
    motts::lox::Chunk chunk;

    chunk.emit<Opcode::true_>(Token{Token_type::true_, "true", 1});
    {
        auto jump_backpatch = chunk.emit_jump_if_false(Token{Token_type::and_, "and", 1});
        chunk.emit_constant(Dynamic_type_value{"true path"}, Token{Token_type::string, "\"true path\"", 1});
        chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
        jump_backpatch.to_next_opcode();

        chunk.emit_constant(Dynamic_type_value{"fall through"}, Token{Token_type::string, "\"fall through\"", 1});
        chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    }

    chunk.emit<Opcode::false_>(Token{Token_type::false_, "false", 1});
    {
        auto jump_backpatch = chunk.emit_jump_if_false(Token{Token_type::and_, "and", 1});
        chunk.emit_constant(Dynamic_type_value{"true path"}, Token{Token_type::string, "\"true path\"", 1});
        chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
        jump_backpatch.to_next_opcode();

        chunk.emit_constant(Dynamic_type_value{"fall through"}, Token{Token_type::string, "\"fall through\"", 1});
        chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    }

    std::ostringstream os;
    motts::lox::run(chunk, os);

    const auto expected =
        "true path\n"
        "fall through\n"
        "fall through\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(jump_will_run) {
    motts::lox::Chunk chunk;

    auto jump_backpatch = chunk.emit_jump(Token{Token_type::or_, "or", 1});
    chunk.emit_constant(Dynamic_type_value{"skip"}, Token{Token_type::string, "\"skip\"", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    jump_backpatch.to_next_opcode();

    chunk.emit_constant(Dynamic_type_value{"fall through"}, Token{Token_type::string, "\"fall through\"", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os);

    BOOST_TEST(os.str() == "fall through\n");
}

BOOST_AUTO_TEST_CASE(set_get_global_will_run) {
    motts::lox::Chunk chunk;
    chunk.emit<Opcode::nil>(Token{Token_type::var, "var", 1});
    chunk.emit<Opcode::define_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::var, "var", 1});
    chunk.emit_constant(Dynamic_type_value{42.0}, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::set_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::identifier, "x", 1});
    chunk.emit<Opcode::pop>(Token{Token_type::semicolon, ";", 1});
    chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::identifier, "x", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os);

    BOOST_TEST(os.str() == "42\n");
}

BOOST_AUTO_TEST_CASE(get_global_of_undeclared_will_throw) {
    motts::lox::Chunk chunk;
    chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::identifier, "x", 1});

    BOOST_CHECK_THROW(motts::lox::run(chunk), std::exception);
    try {
        motts::lox::run(chunk);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error: Undefined variable 'x'.");
    }
}

BOOST_AUTO_TEST_CASE(set_global_of_undeclared_will_throw) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(Dynamic_type_value{42.0}, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::set_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::identifier, "x", 1});

    BOOST_CHECK_THROW(motts::lox::run(chunk), std::exception);
    try {
        motts::lox::run(chunk);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error: Undefined variable 'x'.");
    }
}

BOOST_AUTO_TEST_CASE(vm_state_can_persist_across_multiple_runs) {
    motts::lox::VM vm;
    std::ostringstream os;

    {
        motts::lox::Chunk chunk;
        chunk.emit<Opcode::nil>(Token{Token_type::var, "var", 1});
        chunk.emit<Opcode::define_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::var, "var", 1});
        chunk.emit_constant(Dynamic_type_value{42.0}, Token{Token_type::number, "42", 1});
        chunk.emit<Opcode::set_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::identifier, "x", 1});
        chunk.emit<Opcode::pop>(Token{Token_type::semicolon, ";", 1});
        vm.run(chunk, os);
    }
    {
        motts::lox::Chunk chunk;
        chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::identifier, "x", 1});
        chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
        vm.run(chunk, os);
    }

    BOOST_TEST(os.str() == "42\n");
}

BOOST_AUTO_TEST_CASE(while_will_run) {
    const auto chunk = motts::lox::compile("var x = 3; while (x > 0) print x = x - 1;");

    std::ostringstream os;
    motts::lox::run(chunk, os);

    BOOST_TEST(os.str() == "2\n1\n0\n");
}

BOOST_AUTO_TEST_CASE(global_var_declaration_will_run) {
    motts::lox::Chunk chunk;
    chunk.emit<Opcode::nil>(Token{Token_type::var, "var", 1});
    chunk.emit<Opcode::define_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::var, "var", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os);

    BOOST_TEST(os.str() == "");
}

BOOST_AUTO_TEST_CASE(global_var_will_initialize_from_stack) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(Dynamic_type_value{42.0}, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::define_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::var, "var", 1});
    chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::identifier, "x", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os, /* debug = */ true);

    const auto expected =
        "# Running chunk:\n"
        "Constants:\n"
        "    0 : 42\n"
        "    1 : x\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 14 01    DEFINE_GLOBAL [1]       ; var @ 1\n"
        "    4 : 12 01    GET_GLOBAL [1]          ; x @ 1\n"
        "    6 : 05       PRINT                   ; print @ 1\n"
        "\n"
        "Stack:\n"
        "    0 : 42\n"
        "\n"
        "Stack:\n"
        "\n"
        "Stack:\n"
        "    0 : 42\n"
        "\n"
        "42\n"
        "Stack:\n"
        "\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(local_var_will_get_from_stack) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(Dynamic_type_value{42.0}, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::get_local>(0, Token{Token_type::identifier, "x", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os, /* debug = */ true);

    const auto expected =
        "# Running chunk:\n"
        "Constants:\n"
        "    0 : 42\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 15 00    GET_LOCAL [0]           ; x @ 1\n"
        "\n"
        "Stack:\n"
        "    0 : 42\n"
        "\n"
        "Stack:\n"
        "    1 : 42\n"
        "    0 : 42\n"
        "\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(local_var_will_set_to_stack) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(Dynamic_type_value{28.0}, Token{Token_type::number, "28", 1});
    chunk.emit_constant(Dynamic_type_value{14.0}, Token{Token_type::number, "14", 1});
    chunk.emit<Opcode::set_local>(0, Token{Token_type::identifier, "x", 1});

    std::ostringstream os;
    motts::lox::run(chunk, os, /* debug = */ true);

    const auto expected =
        "# Running chunk:\n"
        "Constants:\n"
        "    0 : 28\n"
        "    1 : 14\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 28 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 14 @ 1\n"
        "    4 : 16 00    SET_LOCAL [0]           ; x @ 1\n"
        "\n"
        "Stack:\n"
        "    0 : 28\n"
        "\n"
        "Stack:\n"
        "    1 : 14\n"
        "    0 : 28\n"
        "\n"
        "Stack:\n"
        "    1 : 14\n"
        "    0 : 14\n"
        "\n";
    BOOST_TEST(os.str() == expected);}
