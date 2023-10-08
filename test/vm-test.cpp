#define BOOST_TEST_MODULE VM Tests

#include <sstream>
#include <stdexcept>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "../src/compiler.hpp"
#include "../src/interned_strings.hpp"
#include "../src/lox.hpp"
#include "../src/object.hpp"
#include "../src/vm.hpp"

using motts::lox::Opcode;
using Token = motts::lox::Chunk::Token;
using motts::lox::Token_type;

BOOST_AUTO_TEST_CASE(vm_will_run_chunks_of_bytecode)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;
    chunk.emit_constant(28.0, Token{Token_type::number, interned_strings.get(std::string_view{"28"}), 1});
    chunk.emit_constant(14.0, Token{Token_type::number, interned_strings.get(std::string_view{"14"}), 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, interned_strings.get(std::string_view{"+"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};
    vm.run(chunk);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n\n"
        "Constants:\n"
        "    0 : 28\n"
        "    1 : 14\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 28 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 14 @ 1\n"
        "    4 : 12       ADD                     ; + @ 1\n"
        "\n"
        "# Stack:\n"
        "    0 : 28\n"
        "\n"
        "# Stack:\n"
        "    1 : 14\n"
        "    0 : 28\n"
        "\n"
        "# Stack:\n"
        "    0 : 42\n"
        "\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(chunks_and_stacks_wont_print_when_debug_is_off)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;
    chunk.emit_constant(28.0, Token{Token_type::number, interned_strings.get(std::string_view{"28"}), 1});
    chunk.emit_constant(14.0, Token{Token_type::number, interned_strings.get(std::string_view{"14"}), 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, interned_strings.get(std::string_view{"+"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "");
}

BOOST_AUTO_TEST_CASE(numbers_and_strings_add)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;
    chunk.emit_constant(28.0, Token{Token_type::number, interned_strings.get(std::string_view{"28"}), 1});
    chunk.emit_constant(14.0, Token{Token_type::number, interned_strings.get(std::string_view{"14"}), 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, interned_strings.get(std::string_view{"+"}), 1});
    chunk.emit_constant(
        interned_strings.get(std::string_view{"hello"}),
        Token{Token_type::number, interned_strings.get(std::string_view{"\"hello\""}), 1}
    );
    chunk.emit_constant(
        interned_strings.get(std::string_view{"world"}),
        Token{Token_type::number, interned_strings.get(std::string_view{"\"world\""}), 1}
    );
    chunk.emit<Opcode::add>(Token{Token_type::plus, interned_strings.get(std::string_view{"+"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};
    vm.run(chunk);

    // clang-format off
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
        "    4 : 12       ADD                     ; + @ 1\n"
        "    5 : 00 02    CONSTANT [2]            ; \"hello\" @ 1\n"
        "    7 : 00 03    CONSTANT [3]            ; \"world\" @ 1\n"
        "    9 : 12       ADD                     ; + @ 1\n"
        "\n"
        "# Stack:\n"
        "    0 : 28\n"
        "\n"
        "# Stack:\n"
        "    1 : 14\n"
        "    0 : 28\n"
        "\n"
        "# Stack:\n"
        "    0 : 42\n"
        "\n"
        "# Stack:\n"
        "    1 : hello\n"
        "    0 : 42\n"
        "\n"
        "# Stack:\n"
        "    2 : world\n"
        "    1 : hello\n"
        "    0 : 42\n"
        "\n"
        "# Stack:\n"
        "    1 : helloworld\n"
        "    0 : 42\n"
        "\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(invalid_plus_will_throw)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;
    chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    chunk.emit_constant(
        interned_strings.get(std::string_view{"hello"}),
        Token{Token_type::number, interned_strings.get(std::string_view{"\"hello\""}), 1}
    );
    chunk.emit<Opcode::add>(Token{Token_type::plus, interned_strings.get(std::string_view{"+"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};

    BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
    try {
        vm.run(chunk);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"+\": Operands must be two numbers or two strings.");
    }
}

BOOST_AUTO_TEST_CASE(print_whats_on_top_of_stack)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;

    chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(
        interned_strings.get(std::string_view{"hello"}),
        Token{Token_type::string, interned_strings.get(std::string_view{"hello"}), 1}
    );
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(nullptr, Token{Token_type::nil, interned_strings.get(std::string_view{"nil"}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(true, Token{Token_type::true_, interned_strings.get(std::string_view{"true"}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(false, Token{Token_type::false_, interned_strings.get(std::string_view{"false"}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(chunk);

    // clang-format off
    const auto expected =
        "42\n"
        "hello\n"
        "nil\n"
        "true\n"
        "false\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(plus_minus_star_slash_will_run)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;

    chunk.emit_constant(7.0, Token{Token_type::number, interned_strings.get(std::string_view{"7"}), 1});
    chunk.emit_constant(5.0, Token{Token_type::number, interned_strings.get(std::string_view{"5"}), 1});
    chunk.emit<Opcode::add>(Token{Token_type::plus, interned_strings.get(std::string_view{"+"}), 1});

    chunk.emit_constant(3.0, Token{Token_type::number, interned_strings.get(std::string_view{"3"}), 1});
    chunk.emit_constant(2.0, Token{Token_type::number, interned_strings.get(std::string_view{"2"}), 1});
    chunk.emit<Opcode::multiply>(Token{Token_type::star, interned_strings.get(std::string_view{"*"}), 1});

    chunk.emit_constant(1.0, Token{Token_type::number, interned_strings.get(std::string_view{"1"}), 1});
    chunk.emit<Opcode::divide>(Token{Token_type::slash, interned_strings.get(std::string_view{"/"}), 1});

    chunk.emit<Opcode::subtract>(Token{Token_type::minus, interned_strings.get(std::string_view{"-"}), 1});

    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "6\n");
}

BOOST_AUTO_TEST_CASE(invalid_plus_minus_star_slash_will_throw)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
        chunk.emit_constant(true, Token{Token_type::true_, interned_strings.get(std::string_view{"true"}), 1});
        chunk.emit<Opcode::add>(Token{Token_type::plus, interned_strings.get(std::string_view{"+"}), 1});

        BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
        try {
            vm.run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \"+\": Operands must be two numbers or two strings.");
        }
    }
    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
        chunk.emit_constant(true, Token{Token_type::true_, interned_strings.get(std::string_view{"true"}), 1});
        chunk.emit<Opcode::subtract>(Token{Token_type::minus, interned_strings.get(std::string_view{"-"}), 1});

        BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
        try {
            vm.run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \"-\": Operands must be numbers.");
        }
    }
    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
        chunk.emit_constant(true, Token{Token_type::true_, interned_strings.get(std::string_view{"true"}), 1});
        chunk.emit<Opcode::multiply>(Token{Token_type::star, interned_strings.get(std::string_view{"*"}), 1});

        BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
        try {
            vm.run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \"*\": Operands must be numbers.");
        }
    }
    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
        chunk.emit_constant(true, Token{Token_type::true_, interned_strings.get(std::string_view{"true"}), 1});
        chunk.emit<Opcode::divide>(Token{Token_type::slash, interned_strings.get(std::string_view{"/"}), 1});

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
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;

    chunk.emit_constant(1.0, Token{Token_type::number, interned_strings.get(std::string_view{"1"}), 1});
    chunk.emit<Opcode::negate>(Token{Token_type::minus, interned_strings.get(std::string_view{"-"}), 1});

    chunk.emit_constant(1.0, Token{Token_type::number, interned_strings.get(std::string_view{"1"}), 1});
    chunk.emit<Opcode::negate>(Token{Token_type::minus, interned_strings.get(std::string_view{"-"}), 1});

    chunk.emit<Opcode::add>(Token{Token_type::plus, interned_strings.get(std::string_view{"+"}), 1});

    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "-2\n");
}

BOOST_AUTO_TEST_CASE(invalid_negation_will_throw)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;
    chunk.emit_constant(
        interned_strings.get(std::string_view{"hello"}),
        Token{Token_type::string, interned_strings.get(std::string_view{"\"hello\""}), 1}
    );
    chunk.emit<Opcode::negate>(Token{Token_type::minus, interned_strings.get(std::string_view{"-"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};

    BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
    try {
        vm.run(chunk);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"-\": Operand must be a number.");
    }
}

BOOST_AUTO_TEST_CASE(false_and_nil_are_falsey_all_else_is_truthy)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;

    chunk.emit_constant(false, Token{Token_type::false_, interned_strings.get(std::string_view{"false"}), 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, interned_strings.get(std::string_view{"!"}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(nullptr, Token{Token_type::nil, interned_strings.get(std::string_view{"nil"}), 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, interned_strings.get(std::string_view{"!"}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(true, Token{Token_type::true_, interned_strings.get(std::string_view{"true"}), 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, interned_strings.get(std::string_view{"!"}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(1.0, Token{Token_type::number, interned_strings.get(std::string_view{"1"}), 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, interned_strings.get(std::string_view{"!"}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(0.0, Token{Token_type::number, interned_strings.get(std::string_view{"0"}), 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang, interned_strings.get(std::string_view{"!"}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(
        interned_strings.get(std::string_view{"hello"}),
        Token{Token_type::string, interned_strings.get(std::string_view{"\"hello\""}), 1}
    );
    chunk.emit<Opcode::not_>(Token{Token_type::bang, interned_strings.get(std::string_view{"!"}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "true\ntrue\nfalse\nfalse\nfalse\nfalse\n");
}

BOOST_AUTO_TEST_CASE(pop_will_run)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;
    chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    chunk.emit<Opcode::pop>(Token{Token_type::semicolon, interned_strings.get(std::string_view{";"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};
    vm.run(chunk);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n\n"
        "Constants:\n"
        "    0 : 42\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 04       POP                     ; ; @ 1\n"
        "\n"
        "# Stack:\n"
        "    0 : 42\n"
        "\n"
        "# Stack:\n"
        "\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(comparisons_will_run)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;

    chunk.emit_constant(1.0, Token{Token_type::number, interned_strings.get(std::string_view{"1"}), 1});
    chunk.emit_constant(2.0, Token{Token_type::number, interned_strings.get(std::string_view{"2"}), 1});
    chunk.emit<Opcode::greater>(Token{Token_type::greater, interned_strings.get(std::string_view{">"}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(3.0, Token{Token_type::number, interned_strings.get(std::string_view{"3"}), 1});
    chunk.emit_constant(5.0, Token{Token_type::number, interned_strings.get(std::string_view{"5"}), 1});
    chunk.emit<Opcode::less>(Token{Token_type::greater_equal, interned_strings.get(std::string_view{">="}), 1});
    chunk.emit<Opcode::not_>(Token{Token_type::greater_equal, interned_strings.get(std::string_view{">="}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(7.0, Token{Token_type::number, interned_strings.get(std::string_view{"7"}), 1});
    chunk.emit_constant(11.0, Token{Token_type::number, interned_strings.get(std::string_view{"11"}), 1});
    chunk.emit<Opcode::equal>(Token{Token_type::equal_equal, interned_strings.get(std::string_view{"=="}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(13.0, Token{Token_type::number, interned_strings.get(std::string_view{"13"}), 1});
    chunk.emit_constant(17.0, Token{Token_type::number, interned_strings.get(std::string_view{"17"}), 1});
    chunk.emit<Opcode::equal>(Token{Token_type::bang_equal, interned_strings.get(std::string_view{"!="}), 1});
    chunk.emit<Opcode::not_>(Token{Token_type::bang_equal, interned_strings.get(std::string_view{"!="}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(19.0, Token{Token_type::number, interned_strings.get(std::string_view{"19"}), 1});
    chunk.emit_constant(23.0, Token{Token_type::number, interned_strings.get(std::string_view{"23"}), 1});
    chunk.emit<Opcode::greater>(Token{Token_type::less_equal, interned_strings.get(std::string_view{"<="}), 1});
    chunk.emit<Opcode::not_>(Token{Token_type::less_equal, interned_strings.get(std::string_view{"<="}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(29.0, Token{Token_type::number, interned_strings.get(std::string_view{"29"}), 1});
    chunk.emit_constant(31.0, Token{Token_type::number, interned_strings.get(std::string_view{"31"}), 1});
    chunk.emit<Opcode::less>(Token{Token_type::less, interned_strings.get(std::string_view{"<"}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(
        interned_strings.get(std::string_view{"42"}),
        Token{Token_type::string, interned_strings.get(std::string_view{"\"42\""}), 1}
    );
    chunk.emit_constant(
        interned_strings.get(std::string_view{"42"}),
        Token{Token_type::string, interned_strings.get(std::string_view{"\"42\""}), 1}
    );
    chunk.emit<Opcode::equal>(Token{Token_type::equal_equal, interned_strings.get(std::string_view{"=="}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    chunk.emit_constant(
        interned_strings.get(std::string_view{"42"}),
        Token{Token_type::string, interned_strings.get(std::string_view{"\"42\""}), 1}
    );
    chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    chunk.emit<Opcode::equal>(Token{Token_type::equal_equal, interned_strings.get(std::string_view{"=="}), 1});
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "false\nfalse\nfalse\ntrue\ntrue\ntrue\ntrue\nfalse\n");
}

BOOST_AUTO_TEST_CASE(invalid_comparisons_will_throw)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(1.0, Token{Token_type::number, interned_strings.get(std::string_view{"1"}), 1});
        chunk.emit_constant(true, Token{Token_type::true_, interned_strings.get(std::string_view{"true"}), 1});
        chunk.emit<Opcode::greater>(Token{Token_type::greater, interned_strings.get(std::string_view{">"}), 1});

        BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
        try {
            vm.run(chunk);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \">\": Operands must be numbers.");
        }
    }
    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(
            interned_strings.get(std::string_view{"hello"}),
            Token{Token_type::string, interned_strings.get(std::string_view{"\"hello\""}), 1}
        );
        chunk.emit_constant(1.0, Token{Token_type::number, interned_strings.get(std::string_view{"1"}), 1});
        chunk.emit<Opcode::less>(Token{Token_type::less, interned_strings.get(std::string_view{"<"}), 1});

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
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;

    chunk.emit<Opcode::true_>(Token{Token_type::true_, interned_strings.get(std::string_view{"true"}), 1});
    {
        auto jump_backpatch = chunk.emit_jump_if_false(Token{Token_type::and_, interned_strings.get(std::string_view{"and"}), 1});
        chunk.emit_constant(
            interned_strings.get(std::string_view{"if true"}),
            Token{Token_type::string, interned_strings.get(std::string_view{"\"if true\""}), 1}
        );
        chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
        jump_backpatch.to_next_opcode();

        chunk.emit_constant(
            interned_strings.get(std::string_view{"if end"}),
            Token{Token_type::string, interned_strings.get(std::string_view{"\"if end\""}), 1}
        );
        chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
    }

    chunk.emit<Opcode::false_>(Token{Token_type::false_, interned_strings.get(std::string_view{"false"}), 1});
    {
        auto jump_backpatch = chunk.emit_jump_if_false(Token{Token_type::and_, interned_strings.get(std::string_view{"and"}), 1});
        chunk.emit_constant(
            interned_strings.get(std::string_view{"if true"}),
            Token{Token_type::string, interned_strings.get(std::string_view{"\"if true\""}), 1}
        );
        chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
        jump_backpatch.to_next_opcode();

        chunk.emit_constant(
            interned_strings.get(std::string_view{"if end"}),
            Token{Token_type::string, interned_strings.get(std::string_view{"\"if end\""}), 1}
        );
        chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
    }

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(chunk);

    // clang-format off
    const auto expected =
        "if true\n"
        "if end\n"
        "if end\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(jump_will_run)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;

    auto jump_backpatch = chunk.emit_jump(Token{Token_type::or_, interned_strings.get(std::string_view{"or"}), 1});
    chunk.emit_constant(
        interned_strings.get(std::string_view{"skip"}),
        Token{Token_type::string, interned_strings.get(std::string_view{"\"skip\""}), 1}
    );
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
    jump_backpatch.to_next_opcode();

    chunk.emit_constant(
        interned_strings.get(std::string_view{"end"}),
        Token{Token_type::string, interned_strings.get(std::string_view{"\"end\""}), 1}
    );
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "end\n");
}

BOOST_AUTO_TEST_CASE(if_else_will_leave_clean_stack)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};

    const auto chunk = compile(gc_heap, interned_strings, "if (true) nil;");
    vm.run(chunk);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n\n"
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; true @ 1\n"
        "    1 : 1a 00 06 JUMP_IF_FALSE +6 -> 10  ; if @ 1\n"
        "    4 : 04       POP                     ; if @ 1\n"
        "    5 : 01       NIL                     ; nil @ 1\n"
        "    6 : 04       POP                     ; ; @ 1\n"
        "    7 : 19 00 01 JUMP +1 -> 11           ; if @ 1\n"
        "   10 : 04       POP                     ; if @ 1\n"
        "\n"
        "# Stack:\n"
        "    0 : true\n"
        "\n"
        "# Stack:\n"
        "    0 : true\n"
        "\n"
        "# Stack:\n"
        "\n"
        "# Stack:\n"
        "    0 : nil\n"
        "\n"
        "# Stack:\n"
        "\n"
        "# Stack:\n"
        "\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(set_get_global_will_run)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;
    chunk.emit<Opcode::nil>(Token{Token_type::var, interned_strings.get(std::string_view{"var"}), 1});
    chunk.emit<Opcode::define_global>(
        interned_strings.get(std::string_view{"x"}),
        Token{Token_type::var, interned_strings.get(std::string_view{"var"}), 1}
    );
    chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    chunk.emit<Opcode::set_global>(
        interned_strings.get(std::string_view{"x"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"x"}), 1}
    );
    chunk.emit<Opcode::pop>(Token{Token_type::semicolon, interned_strings.get(std::string_view{";"}), 1});
    chunk.emit<Opcode::get_global>(
        interned_strings.get(std::string_view{"x"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"x"}), 1}
    );
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "42\n");
}

BOOST_AUTO_TEST_CASE(get_global_of_undeclared_will_throw)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;
    chunk.emit<Opcode::get_global>(
        interned_strings.get(std::string_view{"x"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"x"}), 1}
    );

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};

    BOOST_CHECK_THROW(vm.run(chunk), std::runtime_error);
    try {
        vm.run(chunk);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error: Undefined variable \"x\".");
    }
}

BOOST_AUTO_TEST_CASE(set_global_of_undeclared_will_throw)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;
    chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    chunk.emit<Opcode::set_global>(
        interned_strings.get(std::string_view{"x"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"x"}), 1}
    );

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};

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
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    {
        motts::lox::Chunk chunk;
        chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
        chunk.emit<Opcode::define_global>(
            interned_strings.get(std::string_view{"x"}),
            Token{Token_type::var, interned_strings.get(std::string_view{"var"}), 1}
        );

        vm.run(chunk);
    }
    {
        motts::lox::Chunk chunk;
        chunk.emit<Opcode::get_global>(
            interned_strings.get(std::string_view{"x"}),
            Token{Token_type::identifier, interned_strings.get(std::string_view{"x"}), 1}
        );
        chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

        vm.run(chunk);
    }

    BOOST_TEST(os.str() == "42\n");
}

BOOST_AUTO_TEST_CASE(loop_will_run)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;

    chunk.emit_constant(true, Token{Token_type::true_, interned_strings.get(std::string_view{"true"}), 1});
    const auto condition_begin_bytecode_index = chunk.bytecode().size();
    auto jump_backpatch = chunk.emit_jump_if_false(Token{Token_type::while_, interned_strings.get(std::string_view{"while"}), 1});

    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
    chunk.emit_constant(false, Token{Token_type::false_, interned_strings.get(std::string_view{"false"}), 1});
    chunk.emit_loop(condition_begin_bytecode_index, Token{Token_type::while_, interned_strings.get(std::string_view{"while"}), 1});

    jump_backpatch.to_next_opcode();
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "true\nfalse\n");
}

BOOST_AUTO_TEST_CASE(global_var_declaration_will_run)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;
    chunk.emit<Opcode::nil>(Token{Token_type::var, interned_strings.get(std::string_view{"var"}), 1});
    chunk.emit<Opcode::define_global>(
        interned_strings.get(std::string_view{"x"}),
        Token{Token_type::var, interned_strings.get(std::string_view{"var"}), 1}
    );

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(chunk);

    BOOST_TEST(os.str() == "");
}

BOOST_AUTO_TEST_CASE(global_var_will_initialize_from_stack)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;
    chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    chunk.emit<Opcode::define_global>(
        interned_strings.get(std::string_view{"x"}),
        Token{Token_type::var, interned_strings.get(std::string_view{"var"}), 1}
    );
    chunk.emit<Opcode::get_global>(
        interned_strings.get(std::string_view{"x"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"x"}), 1}
    );
    chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};
    vm.run(chunk);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n\n"
        "Constants:\n"
        "    0 : 42\n"
        "    1 : x\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 08 01    DEFINE_GLOBAL [1]       ; var @ 1\n"
        "    4 : 07 01    GET_GLOBAL [1]          ; x @ 1\n"
        "    6 : 18       PRINT                   ; print @ 1\n"
        "\n"
        "# Stack:\n"
        "    0 : 42\n"
        "\n"
        "# Stack:\n"
        "\n"
        "# Stack:\n"
        "    0 : 42\n"
        "\n"
        "42\n"
        "# Stack:\n"
        "\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(local_var_will_get_from_stack)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;
    chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    chunk.emit<Opcode::get_local>(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"x"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};
    vm.run(chunk);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n\n"
        "Constants:\n"
        "    0 : 42\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 05 00    GET_LOCAL [0]           ; x @ 1\n"
        "\n"
        "# Stack:\n"
        "    0 : 42\n"
        "\n"
        "# Stack:\n"
        "    1 : 42\n"
        "    0 : 42\n"
        "\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(local_var_will_set_to_stack)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;
    chunk.emit_constant(28.0, Token{Token_type::number, interned_strings.get(std::string_view{"28"}), 1});
    chunk.emit_constant(14.0, Token{Token_type::number, interned_strings.get(std::string_view{"14"}), 1});
    chunk.emit<Opcode::set_local>(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"x"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};
    vm.run(chunk);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n\n"
        "Constants:\n"
        "    0 : 28\n"
        "    1 : 14\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 28 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 14 @ 1\n"
        "    4 : 06 00    SET_LOCAL [0]           ; x @ 1\n"
        "\n"
        "# Stack:\n"
        "    0 : 28\n"
        "\n"
        "# Stack:\n"
        "    1 : 14\n"
        "    0 : 28\n"
        "\n"
        "# Stack:\n"
        "    1 : 14\n"
        "    0 : 14\n"
        "\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(call_with_args_will_run)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};

    motts::lox::Chunk fn_f_chunk;
    fn_f_chunk.emit<Opcode::get_local>(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"f"}), 1});
    fn_f_chunk.emit<Opcode::get_local>(1, Token{Token_type::identifier, interned_strings.get(std::string_view{"x"}), 1});
    const auto fn_f = gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"f"}), 1, std::move(fn_f_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit_closure(fn_f, {}, Token{Token_type::fun, interned_strings.get(std::string_view{"fun"}), 1});
    main_chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    main_chunk.emit_call(1, Token{Token_type::identifier, interned_strings.get(std::string_view{"f"}), 1});

    vm.run(main_chunk);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n"
        "\n"
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : 42\n"
        "Bytecode:\n"
        "    0 : 1f 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 00 01    CONSTANT [1]            ; 42 @ 1\n"
        "    5 : 1c 01    CALL (1)                ; f @ 1\n"
        "[<fn f> chunk]\n"
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 05 00    GET_LOCAL [0]           ; f @ 1\n"
        "    2 : 05 01    GET_LOCAL [1]           ; x @ 1\n"
        "\n"
        "# Stack:\n"
        "    0 : <fn f>\n"
        "\n"
        "# Stack:\n"
        "    1 : 42\n"
        "    0 : <fn f>\n"
        "\n"
        "# Stack:\n"
        "    2 : <fn f>\n"
        "    1 : 42\n"
        "    0 : <fn f>\n"
        "\n"
        "# Stack:\n"
        "    3 : 42\n"
        "    2 : <fn f>\n"
        "    1 : 42\n"
        "    0 : <fn f>\n"
        "\n"
        "# Stack:\n"
        "    3 : 42\n"
        "    2 : <fn f>\n"
        "    1 : 42\n"
        "    0 : <fn f>\n"
        "\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(return_will_jump_to_caller_and_pop_stack)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};

    motts::lox::Chunk fn_f_chunk;
    fn_f_chunk.emit<Opcode::get_local>(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"f"}), 1});
    fn_f_chunk.emit<Opcode::get_local>(1, Token{Token_type::identifier, interned_strings.get(std::string_view{"x"}), 1});
    fn_f_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"return"}), 1});
    const auto fn_f = gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"f"}), 1, std::move(fn_f_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::nil>(Token{Token_type::nil, interned_strings.get(std::string_view{"nil"}), 1}
    ); // Force the call frame's stack offset to matter.
    main_chunk.emit_closure(fn_f, {}, Token{Token_type::fun, interned_strings.get(std::string_view{"fun"}), 1});
    main_chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    main_chunk.emit_call(1, Token{Token_type::identifier, interned_strings.get(std::string_view{"f"}), 1});
    main_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    vm.run(main_chunk);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n"
        "\n"
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : 42\n"
        "Bytecode:\n"
        "    0 : 01       NIL                     ; nil @ 1\n"
        "    1 : 1f 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    4 : 00 01    CONSTANT [1]            ; 42 @ 1\n"
        "    6 : 1c 01    CALL (1)                ; f @ 1\n"
        "    8 : 18       PRINT                   ; print @ 1\n"
        "[<fn f> chunk]\n"
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 05 00    GET_LOCAL [0]           ; f @ 1\n"
        "    2 : 05 01    GET_LOCAL [1]           ; x @ 1\n"
        "    4 : 21       RETURN                  ; return @ 1\n"
        "\n"
        "# Stack:\n"
        "    0 : nil\n"
        "\n"
        "# Stack:\n"
        "    1 : <fn f>\n"
        "    0 : nil\n"
        "\n"
        "# Stack:\n"
        "    2 : 42\n"
        "    1 : <fn f>\n"
        "    0 : nil\n"
        "\n"
        "# Stack:\n"
        "    3 : <fn f>\n"
        "    2 : 42\n"
        "    1 : <fn f>\n"
        "    0 : nil\n"
        "\n"
        "# Stack:\n"
        "    4 : 42\n"
        "    3 : <fn f>\n"
        "    2 : 42\n"
        "    1 : <fn f>\n"
        "    0 : nil\n"
        "\n"
        "# Stack:\n"
        "    1 : 42\n"
        "    0 : nil\n"
        "\n"
        "42\n"
        "# Stack:\n"
        "    0 : nil\n"
        "\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(reachable_function_objects_wont_be_collected)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    vm.run(compile(gc_heap, interned_strings, "fun f() {}"));
    // A failing test will throw std::bad_variant_access: std::get: wrong index for variant.
    vm.run(compile(gc_heap, interned_strings, "f();"));
}

BOOST_AUTO_TEST_CASE(function_calls_will_wrong_arity_will_throw)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    motts::lox::Chunk fn_f_chunk;
    fn_f_chunk.emit<Opcode::nil>(Token{Token_type::fun, interned_strings.get(std::string_view{"fun"}), 1});
    fn_f_chunk.emit<Opcode::return_>(Token{Token_type::fun, interned_strings.get(std::string_view{"fun"}), 1});
    const auto fn_f = gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"f"}), 1, std::move(fn_f_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit_closure(fn_f, {}, Token{Token_type::fun, interned_strings.get(std::string_view{"fun"}), 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"f"}), 1});

    BOOST_CHECK_THROW(vm.run(main_chunk), std::runtime_error);
    try {
        vm.run(main_chunk);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"f\": Expected 1 arguments but got 0.");
    }
}

BOOST_AUTO_TEST_CASE(calling_a_noncallable_will_throw)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;
    chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    chunk.emit_call(0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};

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
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    motts::lox::Chunk fn_inner_get_chunk;
    fn_inner_get_chunk.emit<Opcode::get_upvalue>(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"x"}), 1});
    fn_inner_get_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
    fn_inner_get_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"return"}), 1});
    const auto fn_inner_get =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"get"}), 0, std::move(fn_inner_get_chunk)});

    motts::lox::Chunk fn_inner_set_chunk;
    fn_inner_set_chunk.emit_constant(14.0, Token{Token_type::number, interned_strings.get(std::string_view{"14"}), 1});
    fn_inner_set_chunk.emit<Opcode::set_upvalue>(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"x"}), 1});
    fn_inner_set_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"return"}), 1});
    const auto fn_inner_set =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"set"}), 0, std::move(fn_inner_set_chunk)});

    motts::lox::Chunk fn_middle_chunk;
    fn_middle_chunk.emit_closure(fn_inner_get, {{false, 0}}, Token{Token_type::fun, interned_strings.get(std::string_view{"fun"}), 1});
    fn_middle_chunk.emit<Opcode::set_global>(
        interned_strings.get(std::string_view{"get"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"get"}), 1}
    );
    fn_middle_chunk.emit<Opcode::pop>(Token{Token_type::semicolon, interned_strings.get(std::string_view{";"}), 1});
    fn_middle_chunk.emit_closure(fn_inner_set, {{false, 0}}, Token{Token_type::fun, interned_strings.get(std::string_view{"fun"}), 1});
    fn_middle_chunk.emit<Opcode::set_global>(
        interned_strings.get(std::string_view{"set"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"set"}), 1}
    );
    fn_middle_chunk.emit<Opcode::pop>(Token{Token_type::semicolon, interned_strings.get(std::string_view{";"}), 1});
    fn_middle_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"return"}), 1});
    const auto fn_middle =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"middle"}), 0, std::move(fn_middle_chunk)});

    motts::lox::Chunk fn_outer_chunk;
    fn_outer_chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    fn_outer_chunk.emit_closure(fn_middle, {{1, true}}, Token{Token_type::fun, interned_strings.get(std::string_view{"fun"}), 1});
    fn_outer_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"middle"}), 1});
    fn_outer_chunk.emit<Opcode::pop>(Token{Token_type::semicolon, interned_strings.get(std::string_view{";"}), 1});
    fn_outer_chunk.emit<Opcode::close_upvalue>(Token{Token_type::right_brace, interned_strings.get(std::string_view{"}"}), 1});
    fn_outer_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"return"}), 1});
    const auto fn_outer =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"outer"}), 0, std::move(fn_outer_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::nil>(Token{Token_type::var, interned_strings.get(std::string_view{"var"}), 1});
    main_chunk.emit<Opcode::define_global>(
        interned_strings.get(std::string_view{"get"}),
        Token{Token_type::var, interned_strings.get(std::string_view{"var"}), 1}
    );
    main_chunk.emit<Opcode::nil>(Token{Token_type::var, interned_strings.get(std::string_view{"var"}), 1});
    main_chunk.emit<Opcode::define_global>(
        interned_strings.get(std::string_view{"set"}),
        Token{Token_type::var, interned_strings.get(std::string_view{"var"}), 1}
    );
    main_chunk.emit_closure(fn_outer, {}, Token{Token_type::fun, interned_strings.get(std::string_view{"fun"}), 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"outer"}), 1});
    main_chunk.emit<Opcode::get_global>(
        interned_strings.get(std::string_view{"get"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"get"}), 1}
    );
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"get"}), 1});
    main_chunk.emit<Opcode::get_global>(
        interned_strings.get(std::string_view{"set"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"set"}), 1}
    );
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"set"}), 1});
    main_chunk.emit<Opcode::get_global>(
        interned_strings.get(std::string_view{"get"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"get"}), 1}
    );
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"get"}), 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "42\n14\n");
}

BOOST_AUTO_TEST_CASE(closure_decl_and_capture_can_be_out_of_order)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    motts::lox::Chunk fn_inner_get_chunk;
    fn_inner_get_chunk.emit<Opcode::get_upvalue>(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"x"}), 1});
    fn_inner_get_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
    fn_inner_get_chunk.emit<Opcode::get_upvalue>(1, Token{Token_type::identifier, interned_strings.get(std::string_view{"y"}), 1});
    fn_inner_get_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
    fn_inner_get_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"return"}), 1});
    const auto fn_inner_get =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"get"}), 0, std::move(fn_inner_get_chunk)});

    motts::lox::Chunk fn_outer_chunk;
    fn_outer_chunk.emit<Opcode::nil>(Token{Token_type::var, interned_strings.get(std::string_view{"var"}), 1});
    fn_outer_chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    fn_outer_chunk.emit_constant(14.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    fn_outer_chunk
        .emit_closure(fn_inner_get, {{3, true}, {2, true}}, Token{Token_type::fun, interned_strings.get(std::string_view{"fun"}), 1});
    fn_outer_chunk.emit<Opcode::set_local>(1, Token{Token_type::identifier, interned_strings.get(std::string_view{"closure"}), 1});
    fn_outer_chunk.emit<Opcode::pop>(Token{Token_type::right_brace, interned_strings.get(std::string_view{"}"}), 1});
    fn_outer_chunk.emit<Opcode::close_upvalue>(Token{Token_type::right_brace, interned_strings.get(std::string_view{"}"}), 1});
    fn_outer_chunk.emit<Opcode::close_upvalue>(Token{Token_type::right_brace, interned_strings.get(std::string_view{"}"}), 1});
    fn_outer_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"get"}), 1});
    fn_outer_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"return"}), 1});
    const auto fn_outer =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"outer"}), 0, std::move(fn_outer_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit_closure(fn_outer, {}, Token{Token_type::fun, interned_strings.get(std::string_view{"fun"}), 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"outer"}), 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "14\n42\n");
}

BOOST_AUTO_TEST_CASE(closure_early_return_will_close_upvalues)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    motts::lox::Chunk fn_inner_get_chunk;
    fn_inner_get_chunk.emit<Opcode::get_upvalue>(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"x"}), 1});
    fn_inner_get_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
    fn_inner_get_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"return"}), 1});
    const auto fn_inner_get =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"get"}), 0, std::move(fn_inner_get_chunk)});

    motts::lox::Chunk fn_outer_chunk;
    fn_outer_chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    fn_outer_chunk.emit_closure(fn_inner_get, {{1, true}}, Token{Token_type::fun, interned_strings.get(std::string_view{"fun"}), 1});
    fn_outer_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"return"}), 1});
    const auto fn_outer =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"outer"}), 0, std::move(fn_outer_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit_closure(fn_outer, {}, Token{Token_type::fun, interned_strings.get(std::string_view{"fun"}), 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"outer"}), 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"get"}), 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "42\n");
}

BOOST_AUTO_TEST_CASE(class_will_run)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    motts::lox::Chunk fn_method_chunk;
    fn_method_chunk.emit<Opcode::nil>(Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1});
    fn_method_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"return"}), 1});
    const auto fn_method =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"method"}), 0, std::move(fn_method_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::class_>(
        interned_strings.get(std::string_view{"Klass"}),
        Token{Token_type::class_, interned_strings.get(std::string_view{"class"}), 1}
    );
    main_chunk.emit_closure(fn_method, {}, Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1});
    main_chunk.emit<Opcode::method>(
        interned_strings.get(std::string_view{"method"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1}
    );
    main_chunk.emit<Opcode::define_global>(
        interned_strings.get(std::string_view{"Klass"}),
        Token{Token_type::class_, interned_strings.get(std::string_view{"class"}), 1}
    );

    main_chunk.emit<Opcode::get_global>(
        interned_strings.get(std::string_view{"Klass"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"Klass"}), 1}
    );
    main_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    main_chunk.emit<Opcode::get_global>(
        interned_strings.get(std::string_view{"Klass"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"Klass"}), 1}
    );
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"Klass"}), 1});
    main_chunk.emit<Opcode::define_global>(
        interned_strings.get(std::string_view{"instance"}),
        Token{Token_type::var, interned_strings.get(std::string_view{"var"}), 1}
    );

    main_chunk.emit<Opcode::get_global>(
        interned_strings.get(std::string_view{"instance"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"instance"}), 1}
    );
    main_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    main_chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    main_chunk.emit<Opcode::get_global>(
        interned_strings.get(std::string_view{"instance"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"instance"}), 1}
    );
    main_chunk.emit<Opcode::set_property>(
        interned_strings.get(std::string_view{"property"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"property"}), 1}
    );
    main_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    main_chunk.emit<Opcode::get_global>(
        interned_strings.get(std::string_view{"instance"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"instance"}), 1}
    );
    main_chunk.emit<Opcode::get_property>(
        interned_strings.get(std::string_view{"property"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"property"}), 1}
    );
    main_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "<class Klass>\n<instance Klass>\n42\n42\n");
}

BOOST_AUTO_TEST_CASE(methods_bind_and_can_be_called)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    motts::lox::Chunk fn_method_chunk;
    fn_method_chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    fn_method_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
    fn_method_chunk.emit<Opcode::nil>(Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1});
    fn_method_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"return"}), 1});
    const auto fn_method =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"method"}), 0, std::move(fn_method_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::class_>(
        interned_strings.get(std::string_view{"Klass"}),
        Token{Token_type::class_, interned_strings.get(std::string_view{"class"}), 1}
    );
    main_chunk.emit_closure(fn_method, {}, Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1});
    main_chunk.emit<Opcode::method>(
        interned_strings.get(std::string_view{"method"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1}
    );
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"Klass"}), 1});
    main_chunk.emit<Opcode::get_local>(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"instance"}), 1});
    main_chunk.emit<Opcode::get_property>(
        interned_strings.get(std::string_view{"method"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1}
    );
    main_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
    main_chunk.emit<Opcode::get_local>(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"instance"}), 1});
    main_chunk.emit<Opcode::get_property>(
        interned_strings.get(std::string_view{"method"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1}
    );
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "<fn method>\n42\n");
}

BOOST_AUTO_TEST_CASE(this_can_be_captured_in_closure)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    motts::lox::Chunk fn_inner_chunk;
    fn_inner_chunk.emit<Opcode::get_upvalue>(0, Token{Token_type::this_, interned_strings.get(std::string_view{"this"}), 1});
    fn_inner_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
    fn_inner_chunk.emit<Opcode::nil>(Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1});
    fn_inner_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"return"}), 1});
    const auto fn_inner =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"inner"}), 0, std::move(fn_inner_chunk)});

    motts::lox::Chunk fn_method_chunk;
    fn_method_chunk.emit_closure(fn_inner, {{0, true}}, Token{Token_type::fun, interned_strings.get(std::string_view{"fun"}), 1});
    fn_method_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"return"}), 1});
    const auto fn_method =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"method"}), 0, std::move(fn_method_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::class_>(
        interned_strings.get(std::string_view{"Klass"}),
        Token{Token_type::class_, interned_strings.get(std::string_view{"class"}), 1}
    );
    main_chunk.emit_closure(fn_method, {}, Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1});
    main_chunk.emit<Opcode::method>(
        interned_strings.get(std::string_view{"method"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1}
    );
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"Klass"}), 1});
    main_chunk.emit<Opcode::get_property>(
        interned_strings.get(std::string_view{"method"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1}
    );
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1});
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "<instance Klass>\n");
}

BOOST_AUTO_TEST_CASE(init_method_will_run_when_instance_created)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    motts::lox::Chunk fn_init_chunk;
    fn_init_chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    fn_init_chunk.emit<Opcode::get_local>(0, Token{Token_type::this_, interned_strings.get(std::string_view{"this"}), 1});
    fn_init_chunk.emit<Opcode::set_property>(
        interned_strings.get(std::string_view{"property"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"property"}), 1}
    );
    fn_init_chunk.emit<Opcode::get_local>(0, Token{Token_type::this_, interned_strings.get(std::string_view{"this"}), 1});
    fn_init_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"return"}), 1});
    const auto fn_init = gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"init"}), 0, std::move(fn_init_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::class_>(
        interned_strings.get(std::string_view{"Klass"}),
        Token{Token_type::class_, interned_strings.get(std::string_view{"class"}), 1}
    );
    main_chunk.emit_closure(fn_init, {}, Token{Token_type::identifier, interned_strings.get(std::string_view{"init"}), 1});
    main_chunk.emit<Opcode::method>(
        interned_strings.get(std::string_view{"init"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"init"}), 1}
    );
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"Klass"}), 1});
    main_chunk.emit<Opcode::get_property>(
        interned_strings.get(std::string_view{"property"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"property"}), 1}
    );
    main_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "42\n");
}

BOOST_AUTO_TEST_CASE(class_methods_can_inherit)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    motts::lox::Chunk parent_method_chunk;
    parent_method_chunk.emit_constant(
        interned_strings.get(std::string_view{"Parent"}),
        Token{Token_type::string, interned_strings.get(std::string_view{"\"Parent\""}), 1}
    );
    parent_method_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
    parent_method_chunk.emit<Opcode::nil>(Token{Token_type::identifier, interned_strings.get(std::string_view{"parentMethod"}), 1});
    parent_method_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"parentMethod"}), 1});
    const auto parent_method =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"parentMethod"}), 0, std::move(parent_method_chunk)});

    motts::lox::Chunk child_method_chunk;
    child_method_chunk.emit_constant(
        interned_strings.get(std::string_view{"Child"}),
        Token{Token_type::string, interned_strings.get(std::string_view{"\"Child\""}), 1}
    );
    child_method_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
    child_method_chunk.emit<Opcode::nil>(Token{Token_type::identifier, interned_strings.get(std::string_view{"childMethod"}), 1});
    child_method_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"childMethod"}), 1});
    const auto child_method =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"childMethod"}), 0, std::move(child_method_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::class_>(
        interned_strings.get(std::string_view{"Parent"}),
        Token{Token_type::class_, interned_strings.get(std::string_view{"class"}), 1}
    );
    main_chunk.emit_closure(parent_method, {}, Token{Token_type::identifier, interned_strings.get(std::string_view{"parentMethod1"}), 1});
    main_chunk.emit<Opcode::method>(
        interned_strings.get(std::string_view{"parentMethod1"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"parentMethod1"}), 1}
    );
    main_chunk.emit_closure(parent_method, {}, Token{Token_type::identifier, interned_strings.get(std::string_view{"parentMethod2"}), 1});
    main_chunk.emit<Opcode::method>(
        interned_strings.get(std::string_view{"parentMethod2"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"parentMethod2"}), 1}
    );

    main_chunk.emit<Opcode::class_>(
        interned_strings.get(std::string_view{"Child"}),
        Token{Token_type::class_, interned_strings.get(std::string_view{"class"}), 1}
    );
    main_chunk.emit<Opcode::get_local>(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"Parent"}), 1});
    main_chunk.emit<Opcode::inherit>(Token{Token_type::identifier, interned_strings.get(std::string_view{"Parent"}), 1});
    main_chunk.emit_closure(child_method, {}, Token{Token_type::identifier, interned_strings.get(std::string_view{"childMethod"}), 1});
    main_chunk.emit<Opcode::method>(
        interned_strings.get(std::string_view{"childMethod"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"childMethod"}), 1}
    );
    main_chunk.emit_closure(child_method, {}, Token{Token_type::identifier, interned_strings.get(std::string_view{"parentMethod2"}), 1});
    main_chunk.emit<Opcode::method>(
        interned_strings.get(std::string_view{"parentMethod2"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"parentMethod2"}), 1}
    );
    main_chunk.emit<Opcode::pop>(Token{Token_type::identifier, interned_strings.get(std::string_view{"Parent"}), 1});
    main_chunk.emit<Opcode::pop>(Token{Token_type::identifier, interned_strings.get(std::string_view{"Parent"}), 1});

    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"Child"}), 1});
    main_chunk.emit<Opcode::get_local>(1, Token{Token_type::identifier, interned_strings.get(std::string_view{"instance"}), 1});
    main_chunk.emit<Opcode::get_property>(
        interned_strings.get(std::string_view{"childMethod"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"childMethod"}), 1}
    );
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"childMethod"}), 1});
    main_chunk.emit<Opcode::get_local>(1, Token{Token_type::identifier, interned_strings.get(std::string_view{"instance"}), 1});
    main_chunk.emit<Opcode::get_property>(
        interned_strings.get(std::string_view{"parentMethod1"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"parentMethod1"}), 1}
    );
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"parentMethod1"}), 1});
    main_chunk.emit<Opcode::get_local>(1, Token{Token_type::identifier, interned_strings.get(std::string_view{"instance"}), 1});
    main_chunk.emit<Opcode::get_property>(
        interned_strings.get(std::string_view{"parentMethod2"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"parentMethod2"}), 1}
    );
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"parentMethod2"}), 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "Child\nParent\nChild\n");
}

BOOST_AUTO_TEST_CASE(inheriting_from_a_non_class_will_throw)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    motts::lox::Chunk chunk;
    chunk.emit<Opcode::class_>(
        interned_strings.get(std::string_view{"Klass"}),
        Token{Token_type::class_, interned_strings.get(std::string_view{"class"}), 1}
    );
    chunk.emit_constant(42.0, Token{Token_type::number, interned_strings.get(std::string_view{"42"}), 1});
    chunk.emit<Opcode::inherit>(Token{Token_type::identifier, interned_strings.get(std::string_view{"42"}), 1});

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
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    motts::lox::Chunk parent_method_chunk;
    parent_method_chunk.emit_constant(
        interned_strings.get(std::string_view{"Parent"}),
        Token{Token_type::string, interned_strings.get(std::string_view{"\"Parent\""}), 1}
    );
    parent_method_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
    parent_method_chunk.emit<Opcode::nil>(Token{Token_type::identifier, interned_strings.get(std::string_view{"parentMethod"}), 1});
    parent_method_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"parentMethod"}), 1});
    const auto parent_method =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"parentMethod"}), 0, std::move(parent_method_chunk)});

    motts::lox::Chunk child_method_chunk;
    child_method_chunk.emit_constant(
        interned_strings.get(std::string_view{"Child"}),
        Token{Token_type::string, interned_strings.get(std::string_view{"\"Child\""}), 1}
    );
    child_method_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});
    child_method_chunk.emit<Opcode::get_upvalue>(0, Token{Token_type::super, interned_strings.get(std::string_view{"super"}), 1});
    child_method_chunk.emit<Opcode::get_super>(
        interned_strings.get(std::string_view{"method"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1}
    );
    child_method_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1});
    child_method_chunk.emit<Opcode::nil>(Token{Token_type::identifier, interned_strings.get(std::string_view{"childMethod"}), 1});
    child_method_chunk.emit<Opcode::return_>(Token{Token_type::return_, interned_strings.get(std::string_view{"childMethod"}), 1});
    const auto child_method =
        gc_heap.make<motts::lox::Function>({interned_strings.get(std::string_view{"childMethod"}), 0, std::move(child_method_chunk)});

    motts::lox::Chunk main_chunk;
    main_chunk.emit<Opcode::class_>(
        interned_strings.get(std::string_view{"Parent"}),
        Token{Token_type::class_, interned_strings.get(std::string_view{"class"}), 1}
    );
    main_chunk.emit_closure(parent_method, {}, Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1});
    main_chunk.emit<Opcode::method>(
        interned_strings.get(std::string_view{"method"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1}
    );

    main_chunk.emit<Opcode::class_>(
        interned_strings.get(std::string_view{"Child"}),
        Token{Token_type::class_, interned_strings.get(std::string_view{"class"}), 1}
    );
    main_chunk.emit<Opcode::get_local>(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"Parent"}), 1});
    main_chunk.emit<Opcode::inherit>(Token{Token_type::identifier, interned_strings.get(std::string_view{"Parent"}), 1});
    main_chunk.emit_closure(child_method, {{2, true}}, Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1});
    main_chunk.emit<Opcode::method>(
        interned_strings.get(std::string_view{"method"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1}
    );
    main_chunk.emit<Opcode::pop>(Token{Token_type::identifier, interned_strings.get(std::string_view{"Parent"}), 1});
    main_chunk.emit<Opcode::close_upvalue>(Token{Token_type::identifier, interned_strings.get(std::string_view{"Parent"}), 1});

    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"Child"}), 1});
    main_chunk.emit<Opcode::get_property>(
        interned_strings.get(std::string_view{"method"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1}
    );
    main_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"method"}), 1});

    vm.run(main_chunk);

    BOOST_TEST(os.str() == "Child\nParent\n");
}

BOOST_AUTO_TEST_CASE(native_clock_fn_will_run)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk now_chunk;
    now_chunk.emit<Opcode::get_global>(
        interned_strings.get(std::string_view{"clock"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"clock"}), 1}
    );
    now_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"clock"}), 1});
    now_chunk.emit<Opcode::define_global>(
        interned_strings.get(std::string_view{"now"}),
        Token{Token_type::var, interned_strings.get(std::string_view{"var"}), 1}
    );

    motts::lox::Chunk later_chunk;
    later_chunk.emit<Opcode::get_global>(
        interned_strings.get(std::string_view{"clock"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"clock"}), 1}
    );
    later_chunk.emit_call(0, Token{Token_type::identifier, interned_strings.get(std::string_view{"clock"}), 1});
    later_chunk.emit<Opcode::define_global>(
        interned_strings.get(std::string_view{"later"}),
        Token{Token_type::var, interned_strings.get(std::string_view{"var"}), 1}
    );

    later_chunk.emit<Opcode::get_global>(
        interned_strings.get(std::string_view{"later"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"later"}), 1}
    );
    later_chunk.emit<Opcode::get_global>(
        interned_strings.get(std::string_view{"now"}),
        Token{Token_type::identifier, interned_strings.get(std::string_view{"now"}), 1}
    );
    later_chunk.emit<Opcode::greater>(Token{Token_type::greater, interned_strings.get(std::string_view{">"}), 1});
    later_chunk.emit<Opcode::print>(Token{Token_type::print, interned_strings.get(std::string_view{"print"}), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};

    vm.run(now_chunk);
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
    }
    vm.run(later_chunk);

    BOOST_TEST(os.str() == "true\n");
}
