#define BOOST_TEST_MODULE VM Tests
#include <boost/test/unit_test.hpp>

#include <sstream>
#include <stdexcept>
#include <thread>

#include "../src/compiler.hpp"
#include "../src/lox.hpp"
#include "../src/object.hpp"
#include "../src/vm.hpp"

using motts::lox::Opcode;
using motts::lox::Token;
using motts::lox::Token_type;

BOOST_AUTO_TEST_CASE(vm_will_run_chunks_of_bytecode)
{
    motts::lox::Chunk chunk;
    chunk.emit_constant(28.0, Token{Token_type::number, "28", 1});
    chunk.emit_constant(14.0, Token{Token_type::number, "14", 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os, /* debug = */ true};
    vm.run(chunk);

    const auto expected =
        "\n# Running chunk:\n\n"
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

BOOST_AUTO_TEST_CASE(chunks_and_stacks_wont_print_when_debug_is_off)
{
    motts::lox::Chunk chunk;
    chunk.emit_constant(28.0, Token{Token_type::number, "28", 1});
    chunk.emit_constant(14.0, Token{Token_type::number, "14", 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "");
}

BOOST_AUTO_TEST_CASE(numbers_and_strings_add)
{
    motts::lox::Chunk chunk;
    chunk.emit_constant(28.0, Token{Token_type::number, "28", 1});
    chunk.emit_constant(14.0, Token{Token_type::number, "14", 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});
    chunk.emit_constant("hello", Token{Token_type::number, "\"hello\"", 1});
    chunk.emit_constant("world", Token{Token_type::number, "\"world\"", 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os, /* debug = */ true};
    vm.run(chunk);

    const auto expected =
        "\n# Running chunk:\n\n"
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

BOOST_AUTO_TEST_CASE(invalid_plus_will_throw)
{
    motts::lox::Chunk chunk;
    chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    chunk.emit_constant("hello", Token{Token_type::number, "\"hello\"", 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
    try {
        vm.run(chunk);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"+\": Operands must be two numbers or two strings.");
    }
}

BOOST_AUTO_TEST_CASE(print_whats_on_top_of_stack)
{
    motts::lox::Chunk chunk;

    chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant("hello", Token{Token_type::string, "hello", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(nullptr, Token{Token_type::nil, "nil", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(true, Token{Token_type::true_, "true", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(false, Token{Token_type::false_, "false", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};
    vm.run(chunk);

    const auto expected =
        "42\n"
        "hello\n"
        "nil\n"
        "true\n"
        "false\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(plus_minus_star_slash_will_run)
{
    motts::lox::Chunk chunk;

    chunk.emit_constant(7.0, Token{Token_type::number, "7", 1});
    chunk.emit_constant(5.0, Token{Token_type::number, "5", 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});

    chunk.emit_constant(3.0, Token{Token_type::number, "3", 1});
    chunk.emit_constant(2.0, Token{Token_type::number, "2", 1});
    chunk.emit<Opcode::multiply>(Token{Token_type::star, "*", 1});

    chunk.emit_constant(1.0, Token{Token_type::number, "1", 1});
    chunk.emit<Opcode::divide>(Token{Token_type::slash, "/", 1});

    chunk.emit<Opcode::subtract>(Token{Token_type::minus, "-", 1});

    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "6\n");
}

BOOST_AUTO_TEST_CASE(invalid_plus_minus_star_slash_will_throw)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
        chunk.emit_constant(true, Token{Token_type::true_, "true", 1});
        chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});

        BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
        try {
            vm.run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \"+\": Operands must be two numbers or two strings.");
        }
    }
    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
        chunk.emit_constant(true, Token{Token_type::true_, "true", 1});
        chunk.emit<Opcode::subtract>(Token{Token_type::minus, "-", 1});

        BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
        try {
            vm.run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \"-\": Operands must be numbers.");
        }
    }
    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
        chunk.emit_constant(true, Token{Token_type::true_, "true", 1});
        chunk.emit<Opcode::multiply>(Token{Token_type::star, "*", 1});

        BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
        try {
            vm.run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \"*\": Operands must be numbers.");
        }
    }
    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
        chunk.emit_constant(true, Token{Token_type::true_, "true", 1});
        chunk.emit<Opcode::divide>(Token{Token_type::slash, "/", 1});

        BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
        try {
            vm.run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \"/\": Operands must be numbers.");
        }
    }
}

BOOST_AUTO_TEST_CASE(numeric_negation_will_run)
{
    motts::lox::Chunk chunk;

    chunk.emit_constant(1.0, Token{Token_type::number, "1", 1});
    chunk.emit<Opcode::negate>(Token{Token_type::minus, "-", 1});

    chunk.emit_constant(1.0, Token{Token_type::number, "1", 1});
    chunk.emit<Opcode::negate>(Token{Token_type::minus, "-", 1});

    chunk.emit<Opcode::add>(Token{Token_type::plus, "+", 1});

    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "-2\n");
}

BOOST_AUTO_TEST_CASE(invalid_negation_will_throw)
{
    motts::lox::Chunk chunk;
    chunk.emit_constant("hello", Token{Token_type::string, "\"hello\'", 1});
    chunk.emit<Opcode::negate>(Token{Token_type::minus, "-", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
    try {
        vm.run(chunk);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"-\": Operand must be a number.");
    }
}

BOOST_AUTO_TEST_CASE(false_and_nil_are_falsey_all_else_is_truthy)
{
    motts::lox::Chunk chunk;

    chunk.emit_constant(false, Token{Token_type::false_, "false", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, "!", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(nullptr, Token{Token_type::nil, "nil", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, "!", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(true, Token{Token_type::true_, "true", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, "!", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(1.0, Token{Token_type::number, "1", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, "!", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(0.0, Token{Token_type::number, "0", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, "!", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant("hello", Token{Token_type::string, "\"hello\"", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, "!", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "true\ntrue\nfalse\nfalse\nfalse\nfalse\n");
}

BOOST_AUTO_TEST_CASE(pop_will_run)
{
    motts::lox::Chunk chunk;
    chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::pop>(Token{Token_type::semicolon, ";", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os, /* debug = */ true};
    vm.run(chunk);

    const auto expected =
        "\n# Running chunk:\n\n"
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

BOOST_AUTO_TEST_CASE(comparisons_will_run)
{
    motts::lox::Chunk chunk;

    chunk.emit_constant(1.0, Token{Token_type::number, "1", 1});
    chunk.emit_constant(2.0, Token{Token_type::number, "2", 1});
    chunk.emit<Opcode::greater>(Token{Token_type::greater, ">", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(3.0, Token{Token_type::number, "3", 1});
    chunk.emit_constant(5.0, Token{Token_type::number, "5", 1});
    chunk.emit<Opcode::less>(Token{Token_type::greater_equal, ">=", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::greater_equal, ">=", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(7.0, Token{Token_type::number, "7", 1});
    chunk.emit_constant(11.0, Token{Token_type::number, "11", 1});
    chunk.emit<Opcode::equal>(Token{Token_type::equal_equal, "==", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(13.0, Token{Token_type::number, "13", 1});
    chunk.emit_constant(17.0, Token{Token_type::number, "17", 1});
    chunk.emit<Opcode::equal>(Token{Token_type::bang_equal, "!=", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang_equal, "!=", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(19.0, Token{Token_type::number, "19", 1});
    chunk.emit_constant(23.0, Token{Token_type::number, "23", 1});
    chunk.emit<Opcode::greater>(Token{Token_type::less_equal, "<=", 1});
    chunk.emit<Opcode::not_>(Token{Token_type::less_equal, "<=", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant(29.0, Token{Token_type::number, "29", 1});
    chunk.emit_constant(31.0, Token{Token_type::number, "31", 1});
    chunk.emit<Opcode::less>(Token{Token_type::less, "<", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant("42", Token{Token_type::string, "\"42\"", 1});
    chunk.emit_constant("42", Token{Token_type::string, "\"42\"", 1});
    chunk.emit<Opcode::equal>(Token{Token_type::equal_equal, "==", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    chunk.emit_constant("42", Token{Token_type::string, "\"42\"", 1});
    chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::equal>(Token{Token_type::equal_equal, "==", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "false\nfalse\nfalse\ntrue\ntrue\ntrue\ntrue\nfalse\n");
}

BOOST_AUTO_TEST_CASE(invalid_comparisons_will_throw)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(1.0, Token{Token_type::number, "1", 1});
        chunk.emit_constant(true, Token{Token_type::true_, "true", 1});
        chunk.emit<Opcode::greater>(Token{Token_type::greater, ">", 1});

        BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
        try {
            vm.run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \">\": Operands must be numbers.");
        }
    }
    {
        motts::lox::Chunk chunk;
        chunk.emit_constant("hello", Token{Token_type::string, "\"hello\"", 1});
        chunk.emit_constant(1.0, Token{Token_type::number, "1", 1});
        chunk.emit<Opcode::less>(Token{Token_type::less, "<", 1});

        BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
        try {
            vm.run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \"<\": Operands must be numbers.");
        }
    }
}

BOOST_AUTO_TEST_CASE(jump_if_false_will_run)
{
    motts::lox::Chunk chunk;

    chunk.emit<Opcode::true_>(Token{Token_type::true_, "true", 1});
    {
        auto jump_backpatch = chunk.emit_jump_if_false(Token{Token_type::and_, "and", 1});
        chunk.emit_constant("if true", Token{Token_type::string, "\"if true\"", 1});
        chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
        jump_backpatch.to_next_opcode();

        chunk.emit_constant("if end", Token{Token_type::string, "\"if end\"", 1});
        chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    }

    chunk.emit<Opcode::false_>(Token{Token_type::false_, "false", 1});
    {
        auto jump_backpatch = chunk.emit_jump_if_false(Token{Token_type::and_, "and", 1});
        chunk.emit_constant("if true", Token{Token_type::string, "\"if true\"", 1});
        chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
        jump_backpatch.to_next_opcode();

        chunk.emit_constant("if end", Token{Token_type::string, "\"if end\"", 1});
        chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    }

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};
    vm.run(chunk);

    const auto expected =
        "if true\n"
        "if end\n"
        "if end\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(jump_will_run)
{
    motts::lox::Chunk chunk;

    auto jump_backpatch = chunk.emit_jump(Token{Token_type::or_, "or", 1});
    chunk.emit_constant("skip", Token{Token_type::string, "\"skip\"", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    jump_backpatch.to_next_opcode();

    chunk.emit_constant("end", Token{Token_type::string, "\"end\"", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "end\n");
}

BOOST_AUTO_TEST_CASE(if_else_will_leave_clean_stack)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os, /* debug = */ true};

    const auto chunk = compile(gc_heap, "if (true) nil;");
    vm.run(chunk);

    const auto expected =
        "\n# Running chunk:\n\n"
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; true @ 1\n"
        "    1 : 0f 00 06 JUMP_IF_FALSE +6 -> 10  ; if @ 1\n"
        "    4 : 0b       POP                     ; if @ 1\n"
        "    5 : 01       NIL                     ; nil @ 1\n"
        "    6 : 0b       POP                     ; ; @ 1\n"
        "    7 : 10 00 01 JUMP +1 -> 11           ; if @ 1\n"
        "   10 : 0b       POP                     ; if @ 1\n"
        "\n"
        "Stack:\n"
        "    0 : true\n"
        "\n"
        "Stack:\n"
        "    0 : true\n"
        "\n"
        "Stack:\n"
        "\n"
        "Stack:\n"
        "    0 : nil\n"
        "\n"
        "Stack:\n"
        "\n"
        "Stack:\n"
        "\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(set_get_global_will_run)
{
    motts::lox::Chunk chunk;
    chunk.emit<Opcode::nil>(Token{Token_type::var, "var", 1});
    chunk.emit<Opcode::define_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::var, "var", 1});
    chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::set_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::identifier, "x", 1});
    chunk.emit<Opcode::pop>(Token{Token_type::semicolon, ";", 1});
    chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::identifier, "x", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "42\n");
}

BOOST_AUTO_TEST_CASE(get_global_of_undeclared_will_throw)
{
    motts::lox::Chunk chunk;
    chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::identifier, "x", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
    try {
        vm.run(chunk);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error: Undefined variable \"x\".");
    }
}

BOOST_AUTO_TEST_CASE(set_global_of_undeclared_will_throw)
{
    motts::lox::Chunk chunk;
    chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::set_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::identifier, "x", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
    try {
        vm.run(chunk);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error: Undefined variable \"x\".");
    }
}

BOOST_AUTO_TEST_CASE(vm_state_can_persist_across_multiple_runs)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
        chunk.emit<Opcode::define_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::var, "var", 1});

        vm.run(chunk);
    }
    {
        motts::lox::Chunk chunk;
        chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::identifier, "x", 1});
        chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

        vm.run(chunk);
    }

    BOOST_TEST(os.str() == "42\n");
}

BOOST_AUTO_TEST_CASE(loop_will_run)
{
    motts::lox::Chunk chunk;

    chunk.emit_constant(true, Token{Token_type::true_, "true", 1});
    const auto condition_begin_bytecode_index = chunk.bytecode().size();
    auto jump_backpatch = chunk.emit_jump_if_false(Token{Token_type::while_, "while", 1});

    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    chunk.emit_constant(false, Token{Token_type::false_, "false", 1});
    chunk.emit_loop(condition_begin_bytecode_index, Token{Token_type::while_, "while", 1});

    jump_backpatch.to_next_opcode();
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "true\nfalse\n");
}

BOOST_AUTO_TEST_CASE(global_var_declaration_will_run)
{
    motts::lox::Chunk chunk;
    chunk.emit<Opcode::nil>(Token{Token_type::var, "var", 1});
    chunk.emit<Opcode::define_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::var, "var", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "");
}

BOOST_AUTO_TEST_CASE(global_var_will_initialize_from_stack)
{
    motts::lox::Chunk chunk;
    chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::define_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::var, "var", 1});
    chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "x", 1}, Token{Token_type::identifier, "x", 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os, /* debug = */ true};
    vm.run(chunk);

    const auto expected =
        "\n# Running chunk:\n\n"
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

BOOST_AUTO_TEST_CASE(local_var_will_get_from_stack)
{
    motts::lox::Chunk chunk;
    chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::get_local>(0, Token{Token_type::identifier, "x", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os, /* debug = */ true};
    vm.run(chunk);

    const auto expected =
        "\n# Running chunk:\n\n"
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

BOOST_AUTO_TEST_CASE(local_var_will_set_to_stack)
{
    motts::lox::Chunk chunk;
    chunk.emit_constant(28.0, Token{Token_type::number, "28", 1});
    chunk.emit_constant(14.0, Token{Token_type::number, "14", 1});
    chunk.emit<Opcode::set_local>(0, Token{Token_type::identifier, "x", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os, /* debug = */ true};
    vm.run(chunk);

    const auto expected =
        "\n# Running chunk:\n\n"
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
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(call_with_args_will_run)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os, /* debug = */ true};

    motts::lox::Chunk fn_f_chunk;
    fn_f_chunk.emit<Opcode::get_local>(0, Token{Token_type::identifier, "f", 1});
    fn_f_chunk.emit<Opcode::get_local>(1, Token{Token_type::identifier, "x", 1});
    const auto fn_f = gc_heap.make<motts::lox::Function>({"f", 1, std::move(fn_f_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit_closure(fn_f, {}, Token{Token_type::fun, "fun", 1});
    main_chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    main_chunk.emit_call(1, Token{Token_type::identifier, "f", 1});

    vm.run(main_chunk);

    const auto expected =
        "\n# Running chunk:\n"
        "\n"
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : 42\n"
        "Bytecode:\n"
        "    0 : 19 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 00 01    CONSTANT [1]            ; 42 @ 1\n"
        "    5 : 17 01    CALL (1)                ; f @ 1\n"
        "## <fn f> chunk\n"
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 15 00    GET_LOCAL [0]           ; f @ 1\n"
        "    2 : 15 01    GET_LOCAL [1]           ; x @ 1\n"
        "\n"
        "Stack:\n"
        "    0 : <fn f>\n"
        "\n"
        "Stack:\n"
        "    1 : 42\n"
        "    0 : <fn f>\n"
        "\n"
        "Stack:\n"
        "    1 : 42\n"
        "    0 : <fn f>\n"
        "\n"
        "Stack:\n"
        "    2 : <fn f>\n"
        "    1 : 42\n"
        "    0 : <fn f>\n"
        "\n"
        "Stack:\n"
        "    3 : 42\n"
        "    2 : <fn f>\n"
        "    1 : 42\n"
        "    0 : <fn f>\n"
        "\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(return_will_jump_to_caller_and_pop_stack)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os, /* debug = */ true};

    motts::lox::Chunk fn_f_chunk;
    fn_f_chunk.emit<Opcode::get_local>(0, Token{Token_type::identifier, "f", 1});
    fn_f_chunk.emit<Opcode::get_local>(1, Token{Token_type::identifier, "x", 1});
    fn_f_chunk.emit<Opcode::return_>(Token{Token_type::return_, "return", 1});
    const auto fn_f = gc_heap.make<motts::lox::Function>({"f", 1, std::move(fn_f_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::nil>(Token{Token_type::nil, "nil", 1}); // Force the call frame's stack offset to matter.
    main_chunk.emit_closure(fn_f, {}, Token{Token_type::fun, "fun", 1});
    main_chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    main_chunk.emit_call(1, Token{Token_type::identifier, "f", 1});
    main_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    vm.run(main_chunk);

    const auto expected =
        "\n# Running chunk:\n"
        "\n"
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : 42\n"
        "Bytecode:\n"
        "    0 : 01       NIL                     ; nil @ 1\n"
        "    1 : 19 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    4 : 00 01    CONSTANT [1]            ; 42 @ 1\n"
        "    6 : 17 01    CALL (1)                ; f @ 1\n"
        "    8 : 05       PRINT                   ; print @ 1\n"
        "## <fn f> chunk\n"
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 15 00    GET_LOCAL [0]           ; f @ 1\n"
        "    2 : 15 01    GET_LOCAL [1]           ; x @ 1\n"
        "    4 : 18       RETURN                  ; return @ 1\n"
        "\n"
        "Stack:\n"
        "    0 : nil\n"
        "\n"
        "Stack:\n"
        "    1 : <fn f>\n"
        "    0 : nil\n"
        "\n"
        "Stack:\n"
        "    2 : 42\n"
        "    1 : <fn f>\n"
        "    0 : nil\n"
        "\n"
        "Stack:\n"
        "    2 : 42\n"
        "    1 : <fn f>\n"
        "    0 : nil\n"
        "\n"
        "Stack:\n"
        "    3 : <fn f>\n"
        "    2 : 42\n"
        "    1 : <fn f>\n"
        "    0 : nil\n"
        "\n"
        "Stack:\n"
        "    4 : 42\n"
        "    3 : <fn f>\n"
        "    2 : 42\n"
        "    1 : <fn f>\n"
        "    0 : nil\n"
        "\n"
        "Stack:\n"
        "    1 : 42\n"
        "    0 : nil\n"
        "\n"
        "42\n"
        "Stack:\n"
        "    0 : nil\n"
        "\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(reachable_function_objects_wont_be_collected)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    vm.run(compile(gc_heap, "fun f() {}"));
    // A failing test will throw std::bad_variant_access: std::get: wrong index for variant.
    vm.run(compile(gc_heap, "f();"));
}

BOOST_AUTO_TEST_CASE(function_calls_will_wrong_arity_will_throw)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    motts::lox::Chunk fn_f_chunk;
    fn_f_chunk.emit<Opcode::nil>(Token{Token_type::fun, "fun", 1});
    fn_f_chunk.emit<Opcode::return_>(Token{Token_type::fun, "fun", 1});
    const auto fn_f = gc_heap.make<motts::lox::Function>({"f", 1, std::move(fn_f_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit_closure(fn_f, {}, Token{Token_type::fun, "fun", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "f", 1});

    BOOST_CHECK_THROW(vm.run(main_chunk), std::runtime_error);
    try {
        vm.run(main_chunk);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"f\": Expected 1 arguments but got 0.");
    }
}

BOOST_AUTO_TEST_CASE(calling_a_noncallable_will_throw)
{
    motts::lox::Chunk chunk;
    chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    chunk.emit_call(0, Token{Token_type::number, "42", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
    try {
        vm.run(chunk);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"42\": Can only call functions and classes.");
    }
}

BOOST_AUTO_TEST_CASE(closure_get_set_upvalue_will_run)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    motts::lox::Chunk fn_inner_get_chunk;
    fn_inner_get_chunk.emit<Opcode::get_upvalue>(0, Token{Token_type::identifier, "x", 1});
    fn_inner_get_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    fn_inner_get_chunk.emit<Opcode::return_>(Token{Token_type::return_, "return", 1});
    const auto fn_inner_get = gc_heap.make<motts::lox::Function>({"get", 0, std::move(fn_inner_get_chunk)});

    motts::lox::Chunk fn_inner_set_chunk;
    fn_inner_set_chunk.emit_constant(14.0, Token{Token_type::number, "14", 1});
    fn_inner_set_chunk.emit<Opcode::set_upvalue>(0, Token{Token_type::identifier, "x", 1});
    fn_inner_set_chunk.emit<Opcode::return_>(Token{Token_type::return_, "return", 1});
    const auto fn_inner_set = gc_heap.make<motts::lox::Function>({"set", 0, std::move(fn_inner_set_chunk)});

    motts::lox::Chunk fn_middle_chunk;
    fn_middle_chunk.emit_closure(fn_inner_get, {{false, 0}}, Token{Token_type::fun, "fun", 1});
    fn_middle_chunk.emit<Opcode::set_global>(Token{Token_type::identifier, "get", 1}, Token{Token_type::identifier, "get", 1});
    fn_middle_chunk.emit<Opcode::pop>(Token{Token_type::semicolon, ";", 1});
    fn_middle_chunk.emit_closure(fn_inner_set, {{false, 0}}, Token{Token_type::fun, "fun", 1});
    fn_middle_chunk.emit<Opcode::set_global>(Token{Token_type::identifier, "set", 1}, Token{Token_type::identifier, "set", 1});
    fn_middle_chunk.emit<Opcode::pop>(Token{Token_type::semicolon, ";", 1});
    fn_middle_chunk.emit<Opcode::return_>(Token{Token_type::return_, "return", 1});
    const auto fn_middle = gc_heap.make<motts::lox::Function>({"middle", 0, std::move(fn_middle_chunk)});

    motts::lox::Chunk fn_outer_chunk;
    fn_outer_chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    fn_outer_chunk.emit_closure(fn_middle, {{true, 1}}, Token{Token_type::fun, "fun", 1});
    fn_outer_chunk.emit_call(0, Token{Token_type::identifier, "middle", 1});
    fn_outer_chunk.emit<Opcode::pop>(Token{Token_type::semicolon, ";", 1});
    fn_outer_chunk.emit<Opcode::close_upvalue>(Token{Token_type::right_brace, "}", 1});
    fn_outer_chunk.emit<Opcode::return_>(Token{Token_type::return_, "return", 1});
    const auto fn_outer = gc_heap.make<motts::lox::Function>({"outer", 0, std::move(fn_outer_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::nil>(Token{Token_type::var, "var", 1});
    main_chunk.emit<Opcode::define_global>(Token{Token_type::identifier, "get", 1}, Token{Token_type::var, "var", 1});
    main_chunk.emit<Opcode::nil>(Token{Token_type::var, "var", 1});
    main_chunk.emit<Opcode::define_global>(Token{Token_type::identifier, "set", 1}, Token{Token_type::var, "var", 1});
    main_chunk.emit_closure(fn_outer, {}, Token{Token_type::fun, "fun", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "outer", 1});
    main_chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "get", 1}, Token{Token_type::identifier, "get", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "get", 1});
    main_chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "set", 1}, Token{Token_type::identifier, "set", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "set", 1});
    main_chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "get", 1}, Token{Token_type::identifier, "get", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "get", 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "42\n14\n");
}

BOOST_AUTO_TEST_CASE(closure_decl_and_capture_can_be_out_of_order)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    motts::lox::Chunk fn_inner_get_chunk;
    fn_inner_get_chunk.emit<Opcode::get_upvalue>(0, Token{Token_type::identifier, "x", 1});
    fn_inner_get_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    fn_inner_get_chunk.emit<Opcode::get_upvalue>(1, Token{Token_type::identifier, "y", 1});
    fn_inner_get_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    fn_inner_get_chunk.emit<Opcode::return_>(Token{Token_type::return_, "return", 1});
    const auto fn_inner_get = gc_heap.make<motts::lox::Function>({"get", 0, std::move(fn_inner_get_chunk)});

    motts::lox::Chunk fn_outer_chunk;
    fn_outer_chunk.emit<Opcode::nil>(Token{Token_type::var, "var", 1});
    fn_outer_chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    fn_outer_chunk.emit_constant(14.0, Token{Token_type::number, "42", 1});
    fn_outer_chunk.emit_closure(fn_inner_get, {{true, 3}, {true, 2}}, Token{Token_type::fun, "fun", 1});
    fn_outer_chunk.emit<Opcode::set_local>(1, Token{Token_type::identifier, "closure", 1});
    fn_outer_chunk.emit<Opcode::pop>(Token{Token_type::right_brace, "}", 1});
    fn_outer_chunk.emit<Opcode::close_upvalue>(Token{Token_type::right_brace, "}", 1});
    fn_outer_chunk.emit<Opcode::close_upvalue>(Token{Token_type::right_brace, "}", 1});
    fn_outer_chunk.emit_call(0, Token{Token_type::identifier, "get", 1});
    fn_outer_chunk.emit<Opcode::return_>(Token{Token_type::return_, "return", 1});
    const auto fn_outer = gc_heap.make<motts::lox::Function>({"outer", 0, std::move(fn_outer_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit_closure(fn_outer, {}, Token{Token_type::fun, "fun", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "outer", 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "14\n42\n");
}

BOOST_AUTO_TEST_CASE(closure_early_return_will_close_upvalues)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    motts::lox::Chunk fn_inner_get_chunk;
    fn_inner_get_chunk.emit<Opcode::get_upvalue>(0, Token{Token_type::identifier, "x", 1});
    fn_inner_get_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    fn_inner_get_chunk.emit<Opcode::return_>(Token{Token_type::return_, "return", 1});
    const auto fn_inner_get = gc_heap.make<motts::lox::Function>({"get", 0, std::move(fn_inner_get_chunk)});

    motts::lox::Chunk fn_outer_chunk;
    fn_outer_chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    fn_outer_chunk.emit_closure(fn_inner_get, {{true, 1}}, Token{Token_type::fun, "fun", 1});
    fn_outer_chunk.emit<Opcode::return_>(Token{Token_type::return_, "return", 1});
    const auto fn_outer = gc_heap.make<motts::lox::Function>({"outer", 0, std::move(fn_outer_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit_closure(fn_outer, {}, Token{Token_type::fun, "fun", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "outer", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "get", 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "42\n");
}

BOOST_AUTO_TEST_CASE(class_will_run)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    motts::lox::Chunk fn_method_chunk;
    fn_method_chunk.emit<Opcode::nil>(Token{Token_type::identifier, "method", 1});
    fn_method_chunk.emit<Opcode::return_>(Token{Token_type::return_, "return", 1});
    const auto fn_method = gc_heap.make<motts::lox::Function>({"method", 0, std::move(fn_method_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::class_>(Token{Token_type::identifier, "Klass", 1}, Token{Token_type::class_, "class", 1});
    main_chunk.emit_closure(fn_method, {}, Token{Token_type::identifier, "method", 1});
    main_chunk.emit<Opcode::method>(Token{Token_type::identifier, "method", 1}, Token{Token_type::identifier, "method", 1});
    main_chunk.emit<Opcode::define_global>(Token{Token_type::identifier, "Klass", 1}, Token{Token_type::class_, "class", 1});

    main_chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "Klass", 1}, Token{Token_type::identifier, "Klass", 1});
    main_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    main_chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "Klass", 1}, Token{Token_type::identifier, "Klass", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "Klass", 1});
    main_chunk.emit<Opcode::define_global>(Token{Token_type::identifier, "instance", 1}, Token{Token_type::var, "var", 1});

    main_chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "instance", 1}, Token{Token_type::identifier, "instance", 1});
    main_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    main_chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    main_chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "instance", 1}, Token{Token_type::identifier, "instance", 1});
    main_chunk.emit<Opcode::set_property>(Token{Token_type::identifier, "property", 1}, Token{Token_type::identifier, "property", 1});
    main_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    main_chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "instance", 1}, Token{Token_type::identifier, "instance", 1});
    main_chunk.emit<Opcode::get_property>(Token{Token_type::identifier, "property", 1}, Token{Token_type::identifier, "property", 1});
    main_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "<class Klass>\n<instance Klass>\n42\n42\n");
}

BOOST_AUTO_TEST_CASE(methods_bind_and_can_be_called)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    motts::lox::Chunk fn_method_chunk;
    fn_method_chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    fn_method_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    fn_method_chunk.emit<Opcode::nil>(Token{Token_type::identifier, "method", 1});
    fn_method_chunk.emit<Opcode::return_>(Token{Token_type::return_, "return", 1});
    const auto fn_method = gc_heap.make<motts::lox::Function>({"method", 0, std::move(fn_method_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::class_>(Token{Token_type::identifier, "Klass", 1}, Token{Token_type::class_, "class", 1});
    main_chunk.emit_closure(fn_method, {}, Token{Token_type::identifier, "method", 1});
    main_chunk.emit<Opcode::method>(Token{Token_type::identifier, "method", 1}, Token{Token_type::identifier, "method", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "Klass", 1});
    main_chunk.emit<Opcode::get_local>(0, Token{Token_type::identifier, "instance", 1});
    main_chunk.emit<Opcode::get_property>(Token{Token_type::identifier, "method", 1}, Token{Token_type::identifier, "method", 1});
    main_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    main_chunk.emit<Opcode::get_local>(0, Token{Token_type::identifier, "instance", 1});
    main_chunk.emit<Opcode::get_property>(Token{Token_type::identifier, "method", 1}, Token{Token_type::identifier, "method", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "method", 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "<fn method>\n42\n");
}

BOOST_AUTO_TEST_CASE(this_can_be_captured_in_closure)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    motts::lox::Chunk fn_inner_chunk;
    fn_inner_chunk.emit<Opcode::get_upvalue>(0, Token{Token_type::this_, "this", 1});
    fn_inner_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    fn_inner_chunk.emit<Opcode::nil>(Token{Token_type::identifier, "method", 1});
    fn_inner_chunk.emit<Opcode::return_>(Token{Token_type::return_, "return", 1});
    const auto fn_inner = gc_heap.make<motts::lox::Function>({"inner", 0, std::move(fn_inner_chunk)});

    motts::lox::Chunk fn_method_chunk;
    fn_method_chunk.emit_closure(fn_inner, {{true, 0}}, Token{Token_type::fun, "fun", 1});
    fn_method_chunk.emit<Opcode::return_>(Token{Token_type::return_, "return", 1});
    const auto fn_method = gc_heap.make<motts::lox::Function>({"method", 0, std::move(fn_method_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::class_>(Token{Token_type::identifier, "Klass", 1}, Token{Token_type::class_, "class", 1});
    main_chunk.emit_closure(fn_method, {}, Token{Token_type::identifier, "method", 1});
    main_chunk.emit<Opcode::method>(Token{Token_type::identifier, "method", 1}, Token{Token_type::identifier, "method", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "Klass", 1});
    main_chunk.emit<Opcode::get_property>(Token{Token_type::identifier, "method", 1}, Token{Token_type::identifier, "method", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "method", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "method", 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "<instance Klass>\n");
}

BOOST_AUTO_TEST_CASE(init_method_will_run_when_instance_created)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    motts::lox::Chunk fn_init_chunk;
    fn_init_chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    fn_init_chunk.emit<Opcode::get_local>(0, Token{Token_type::this_, "this", 1});
    fn_init_chunk.emit<Opcode::set_property>(Token{Token_type::identifier, "property", 1}, Token{Token_type::identifier, "property", 1});
    fn_init_chunk.emit<Opcode::get_local>(0, Token{Token_type::this_, "this", 1});
    fn_init_chunk.emit<Opcode::return_>(Token{Token_type::return_, "return", 1});
    const auto fn_init = gc_heap.make<motts::lox::Function>({"init", 0, std::move(fn_init_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::class_>(Token{Token_type::identifier, "Klass", 1}, Token{Token_type::class_, "class", 1});
    main_chunk.emit_closure(fn_init, {}, Token{Token_type::identifier, "init", 1});
    main_chunk.emit<Opcode::method>(Token{Token_type::identifier, "init", 1}, Token{Token_type::identifier, "init", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "Klass", 1});
    main_chunk.emit<Opcode::get_property>(Token{Token_type::identifier, "property", 1}, Token{Token_type::identifier, "property", 1});
    main_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "42\n");
}

BOOST_AUTO_TEST_CASE(class_methods_can_inherit)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    motts::lox::Chunk parent_method_chunk;
    parent_method_chunk.emit_constant("Parent", Token{Token_type::string, "\"Parent\"", 1});
    parent_method_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    parent_method_chunk.emit<Opcode::nil>(Token{Token_type::identifier, "parentMethod", 1});
    parent_method_chunk.emit<Opcode::return_>(Token{Token_type::return_, "parentMethod", 1});
    const auto parent_method = gc_heap.make<motts::lox::Function>({"parentMethod", 0, std::move(parent_method_chunk)});

    motts::lox::Chunk child_method_chunk;
    child_method_chunk.emit_constant("Child", Token{Token_type::string, "\"Child\"", 1});
    child_method_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    child_method_chunk.emit<Opcode::nil>(Token{Token_type::identifier, "childMethod", 1});
    child_method_chunk.emit<Opcode::return_>(Token{Token_type::return_, "childMethod", 1});
    const auto child_method = gc_heap.make<motts::lox::Function>({"childMethod", 0, std::move(child_method_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::class_>(Token{Token_type::identifier, "Parent", 1}, Token{Token_type::class_, "class", 1});
    main_chunk.emit_closure(parent_method, {}, Token{Token_type::identifier, "parentMethod1", 1});
    main_chunk.emit<Opcode::method>(Token{Token_type::identifier, "parentMethod1", 1}, Token{Token_type::identifier, "parentMethod1", 1});
    main_chunk.emit_closure(parent_method, {}, Token{Token_type::identifier, "parentMethod2", 1});
    main_chunk.emit<Opcode::method>(Token{Token_type::identifier, "parentMethod2", 1}, Token{Token_type::identifier, "parentMethod2", 1});

    main_chunk.emit<Opcode::class_>(Token{Token_type::identifier, "Child", 1}, Token{Token_type::class_, "class", 1});
    main_chunk.emit<Opcode::get_local>(0, Token{Token_type::identifier, "Parent", 1});
    main_chunk.emit<Opcode::inherit>(Token{Token_type::identifier, "Parent", 1});
    main_chunk.emit_closure(child_method, {}, Token{Token_type::identifier, "childMethod", 1});
    main_chunk.emit<Opcode::method>(Token{Token_type::identifier, "childMethod", 1}, Token{Token_type::identifier, "childMethod", 1});
    main_chunk.emit_closure(child_method, {}, Token{Token_type::identifier, "parentMethod2", 1});
    main_chunk.emit<Opcode::method>(Token{Token_type::identifier, "parentMethod2", 1}, Token{Token_type::identifier, "parentMethod2", 1});
    main_chunk.emit<Opcode::pop>(Token{Token_type::identifier, "Parent", 1});
    main_chunk.emit<Opcode::pop>(Token{Token_type::identifier, "Parent", 1});

    main_chunk.emit_call(0, Token{Token_type::identifier, "Child", 1});
    main_chunk.emit<Opcode::get_local>(1, Token{Token_type::identifier, "instance", 1});
    main_chunk.emit<Opcode::get_property>(Token{Token_type::identifier, "childMethod", 1}, Token{Token_type::identifier, "childMethod", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "childMethod", 1});
    main_chunk.emit<Opcode::get_local>(1, Token{Token_type::identifier, "instance", 1});
    main_chunk.emit<Opcode::get_property>(Token{Token_type::identifier, "parentMethod1", 1}, Token{Token_type::identifier, "parentMethod1", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "parentMethod1", 1});
    main_chunk.emit<Opcode::get_local>(1, Token{Token_type::identifier, "instance", 1});
    main_chunk.emit<Opcode::get_property>(Token{Token_type::identifier, "parentMethod2", 1}, Token{Token_type::identifier, "parentMethod2", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "parentMethod2", 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "Child\nParent\nChild\n");
}

BOOST_AUTO_TEST_CASE(inheriting_from_a_non_class_will_throw)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    motts::lox::Chunk chunk;
    chunk.emit<Opcode::class_>(Token{Token_type::identifier, "Klass", 1}, Token{Token_type::class_, "class", 1});
    chunk.emit_constant(42.0, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::inherit>(Token{Token_type::identifier, "42", 1});

    BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
    try {
        vm.run(chunk);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"42\": Superclass must be a class.");
    }
}

BOOST_AUTO_TEST_CASE(super_calls_will_run)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    motts::lox::Chunk parent_method_chunk;
    parent_method_chunk.emit_constant("Parent", Token{Token_type::string, "\"Parent\"", 1});
    parent_method_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    parent_method_chunk.emit<Opcode::nil>(Token{Token_type::identifier, "parentMethod", 1});
    parent_method_chunk.emit<Opcode::return_>(Token{Token_type::return_, "parentMethod", 1});
    const auto parent_method = gc_heap.make<motts::lox::Function>({"parentMethod", 0, std::move(parent_method_chunk)});

    motts::lox::Chunk child_method_chunk;
    child_method_chunk.emit_constant("Child", Token{Token_type::string, "\"Child\"", 1});
    child_method_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});
    child_method_chunk.emit<Opcode::get_upvalue>(0, Token{Token_type::super, "super", 1});
    child_method_chunk.emit<Opcode::get_super>(Token{Token_type::identifier, "method", 1}, Token{Token_type::identifier, "method", 1});
    child_method_chunk.emit_call(0, Token{Token_type::identifier, "method", 1});
    child_method_chunk.emit<Opcode::nil>(Token{Token_type::identifier, "childMethod", 1});
    child_method_chunk.emit<Opcode::return_>(Token{Token_type::return_, "childMethod", 1});
    const auto child_method = gc_heap.make<motts::lox::Function>({"childMethod", 0, std::move(child_method_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::class_>(Token{Token_type::identifier, "Parent", 1}, Token{Token_type::class_, "class", 1});
    main_chunk.emit_closure(parent_method, {}, Token{Token_type::identifier, "method", 1});
    main_chunk.emit<Opcode::method>(Token{Token_type::identifier, "method", 1}, Token{Token_type::identifier, "method", 1});

    main_chunk.emit<Opcode::class_>(Token{Token_type::identifier, "Child", 1}, Token{Token_type::class_, "class", 1});
    main_chunk.emit<Opcode::get_local>(0, Token{Token_type::identifier, "Parent", 1});
    main_chunk.emit<Opcode::inherit>(Token{Token_type::identifier, "Parent", 1});
    main_chunk.emit_closure(child_method, {{true, 2}}, Token{Token_type::identifier, "method", 1});
    main_chunk.emit<Opcode::method>(Token{Token_type::identifier, "method", 1}, Token{Token_type::identifier, "method", 1});
    main_chunk.emit<Opcode::pop>(Token{Token_type::identifier, "Parent", 1});
    main_chunk.emit<Opcode::close_upvalue>(Token{Token_type::identifier, "Parent", 1});

    main_chunk.emit_call(0, Token{Token_type::identifier, "Child", 1});
    main_chunk.emit<Opcode::get_property>(Token{Token_type::identifier, "method", 1}, Token{Token_type::identifier, "method", 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, "method", 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "Child\nParent\n");
}

BOOST_AUTO_TEST_CASE(native_clock_fn_will_run)
{
    motts::lox::Chunk now_chunk;
    now_chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "clock", 1}, Token{Token_type::identifier, "clock", 1});
    now_chunk.emit_call(0, Token{Token_type::identifier, "clock", 1});
    now_chunk.emit<Opcode::define_global>(Token{Token_type::identifier, "now", 1}, Token{Token_type::var, "var", 1});

    motts::lox::Chunk later_chunk;
    later_chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "clock", 1}, Token{Token_type::identifier, "clock", 1});
    later_chunk.emit_call(0, Token{Token_type::identifier, "clock", 1});
    later_chunk.emit<Opcode::define_global>(Token{Token_type::identifier, "later", 1}, Token{Token_type::var, "var", 1});

    later_chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "later", 1}, Token{Token_type::identifier, "later", 1});
    later_chunk.emit<Opcode::get_global>(Token{Token_type::identifier, "now", 1}, Token{Token_type::identifier, "now", 1});
    later_chunk.emit<Opcode::greater>(Token{Token_type::greater, ">", 1});
    later_chunk.emit<Opcode::print>(Token{Token_type::print, "print", 1});

    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::VM vm {gc_heap, os};

    vm.run(now_chunk);
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
    }
    vm.run(later_chunk);

    BOOST_TEST(os.str() == "true\n");
}
