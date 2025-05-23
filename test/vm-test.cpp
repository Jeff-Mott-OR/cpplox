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
using Token = motts::lox::Source_map_token;

BOOST_AUTO_TEST_CASE(vm_will_run_chunks_of_bytecode)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    auto root_fn = gc_heap.make<motts::lox::Function>({});
    root_fn->chunk.emit_constant(28.0, Token{interned_strings.get("28"), 1});
    root_fn->chunk.emit_constant(14.0, Token{interned_strings.get("14"), 1});
    root_fn->chunk.emit<Opcode::add>(Token{interned_strings.get("+"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};
    vm.run(root_fn);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 28 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 14 @ 1\n"
        "    4 : 12       ADD                     ; + @ 1\n"
        "Constants:\n"
        "    0 : 28\n"
        "    1 : 14\n"
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

    auto root_fn = gc_heap.make<motts::lox::Function>({});
    root_fn->chunk.emit_constant(28.0, Token{interned_strings.get("28"), 1});
    root_fn->chunk.emit_constant(14.0, Token{interned_strings.get("14"), 1});
    root_fn->chunk.emit<Opcode::add>(Token{interned_strings.get("+"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(root_fn);

    BOOST_TEST(os.str() == "");
}

BOOST_AUTO_TEST_CASE(numbers_and_strings_add)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    auto root_fn = gc_heap.make<motts::lox::Function>({});
    root_fn->chunk.emit_constant(28.0, Token{interned_strings.get("28"), 1});
    root_fn->chunk.emit_constant(14.0, Token{interned_strings.get("14"), 1});
    root_fn->chunk.emit<Opcode::add>(Token{interned_strings.get("+"), 1});
    root_fn->chunk.emit_constant(interned_strings.get("hello"), Token{interned_strings.get("\"hello\""), 1});
    root_fn->chunk.emit_constant(interned_strings.get("world"), Token{interned_strings.get("\"world\""), 1});
    root_fn->chunk.emit<Opcode::add>(Token{interned_strings.get("+"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};
    vm.run(root_fn);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 28 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 14 @ 1\n"
        "    4 : 12       ADD                     ; + @ 1\n"
        "    5 : 00 02    CONSTANT [2]            ; \"hello\" @ 1\n"
        "    7 : 00 03    CONSTANT [3]            ; \"world\" @ 1\n"
        "    9 : 12       ADD                     ; + @ 1\n"
        "Constants:\n"
        "    0 : 28\n"
        "    1 : 14\n"
        "    2 : hello\n"
        "    3 : world\n"
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

    auto root_fn = gc_heap.make<motts::lox::Function>({});
    root_fn->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    root_fn->chunk.emit_constant(interned_strings.get("hello"), Token{interned_strings.get("\"hello\""), 1});
    root_fn->chunk.emit<Opcode::add>(Token{interned_strings.get("+"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};

    BOOST_CHECK_THROW(vm.run(root_fn), std::runtime_error);
    try {
        vm.run(root_fn);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"+\": Operands must be two numbers or two strings.");
    }
}

BOOST_AUTO_TEST_CASE(print_whats_on_top_of_stack)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    auto root_fn = gc_heap.make<motts::lox::Function>({});

    root_fn->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(interned_strings.get("hello"), Token{interned_strings.get("hello"), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(nullptr, Token{interned_strings.get("nil"), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(true, Token{interned_strings.get("true"), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(false, Token{interned_strings.get("false"), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(root_fn);

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

    auto root_fn = gc_heap.make<motts::lox::Function>({});

    root_fn->chunk.emit_constant(7.0, Token{interned_strings.get("7"), 1});
    root_fn->chunk.emit_constant(5.0, Token{interned_strings.get("5"), 1});
    root_fn->chunk.emit<Opcode::add>(Token{interned_strings.get("+"), 1});

    root_fn->chunk.emit_constant(3.0, Token{interned_strings.get("3"), 1});
    root_fn->chunk.emit_constant(2.0, Token{interned_strings.get("2"), 1});
    root_fn->chunk.emit<Opcode::multiply>(Token{interned_strings.get("*"), 1});

    root_fn->chunk.emit_constant(1.0, Token{interned_strings.get("1"), 1});
    root_fn->chunk.emit<Opcode::divide>(Token{interned_strings.get("/"), 1});

    root_fn->chunk.emit<Opcode::subtract>(Token{interned_strings.get("-"), 1});

    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(root_fn);

    BOOST_TEST(os.str() == "6\n");
}

BOOST_AUTO_TEST_CASE(invalid_plus_minus_star_slash_will_throw)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    {
        auto root_fn = gc_heap.make<motts::lox::Function>({});
        root_fn->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
        root_fn->chunk.emit_constant(true, Token{interned_strings.get("true"), 1});
        root_fn->chunk.emit<Opcode::add>(Token{interned_strings.get("+"), 1});

        BOOST_CHECK_THROW(vm.run(root_fn), std::runtime_error);
        try {
            vm.run(root_fn);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \"+\": Operands must be two numbers or two strings.");
        }
    }
    {
        auto root_fn = gc_heap.make<motts::lox::Function>({});
        root_fn->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
        root_fn->chunk.emit_constant(true, Token{interned_strings.get("true"), 1});
        root_fn->chunk.emit<Opcode::subtract>(Token{interned_strings.get("-"), 1});

        BOOST_CHECK_THROW(vm.run(root_fn), std::runtime_error);
        try {
            vm.run(root_fn);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \"-\": Operands must be numbers.");
        }
    }
    {
        auto root_fn = gc_heap.make<motts::lox::Function>({});
        root_fn->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
        root_fn->chunk.emit_constant(true, Token{interned_strings.get("true"), 1});
        root_fn->chunk.emit<Opcode::multiply>(Token{interned_strings.get("*"), 1});

        BOOST_CHECK_THROW(vm.run(root_fn), std::runtime_error);
        try {
            vm.run(root_fn);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \"*\": Operands must be numbers.");
        }
    }
    {
        auto root_fn = gc_heap.make<motts::lox::Function>({});
        root_fn->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
        root_fn->chunk.emit_constant(true, Token{interned_strings.get("true"), 1});
        root_fn->chunk.emit<Opcode::divide>(Token{interned_strings.get("/"), 1});

        BOOST_CHECK_THROW(vm.run(root_fn), std::runtime_error);
        try {
            vm.run(root_fn);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \"/\": Operands must be numbers.");
        }
    }
}

BOOST_AUTO_TEST_CASE(numeric_negation_will_run)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    auto root_fn = gc_heap.make<motts::lox::Function>({});

    root_fn->chunk.emit_constant(1.0, Token{interned_strings.get("1"), 1});
    root_fn->chunk.emit<Opcode::negate>(Token{interned_strings.get("-"), 1});

    root_fn->chunk.emit_constant(1.0, Token{interned_strings.get("1"), 1});
    root_fn->chunk.emit<Opcode::negate>(Token{interned_strings.get("-"), 1});

    root_fn->chunk.emit<Opcode::add>(Token{interned_strings.get("+"), 1});

    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(root_fn);

    BOOST_TEST(os.str() == "-2\n");
}

BOOST_AUTO_TEST_CASE(invalid_negation_will_throw)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    auto root_fn = gc_heap.make<motts::lox::Function>({});
    root_fn->chunk.emit_constant(interned_strings.get("hello"), Token{interned_strings.get("\"hello\""), 1});
    root_fn->chunk.emit<Opcode::negate>(Token{interned_strings.get("-"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};

    BOOST_CHECK_THROW(vm.run(root_fn), std::runtime_error);
    try {
        vm.run(root_fn);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"-\": Operand must be a number.");
    }
}

BOOST_AUTO_TEST_CASE(false_and_nil_are_falsey_all_else_is_truthy)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    auto root_fn = gc_heap.make<motts::lox::Function>({});

    root_fn->chunk.emit_constant(false, Token{interned_strings.get("false"), 1});
    root_fn->chunk.emit<Opcode::not_>(Token{interned_strings.get("!"), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(nullptr, Token{interned_strings.get("nil"), 1});
    root_fn->chunk.emit<Opcode::not_>(Token{interned_strings.get("!"), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(true, Token{interned_strings.get("true"), 1});
    root_fn->chunk.emit<Opcode::not_>(Token{interned_strings.get("!"), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(1.0, Token{interned_strings.get("1"), 1});
    root_fn->chunk.emit<Opcode::not_>(Token{interned_strings.get("!"), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(0.0, Token{interned_strings.get("0"), 1});
    root_fn->chunk.emit<Opcode::not_>(Token{interned_strings.get("!"), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(interned_strings.get("hello"), Token{interned_strings.get("\"hello\""), 1});
    root_fn->chunk.emit<Opcode::not_>(Token{interned_strings.get("!"), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(root_fn);

    BOOST_TEST(os.str() == "true\ntrue\nfalse\nfalse\nfalse\nfalse\n");
}

BOOST_AUTO_TEST_CASE(pop_will_run)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    auto root_fn = gc_heap.make<motts::lox::Function>({});
    root_fn->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    root_fn->chunk.emit<Opcode::pop>(Token{interned_strings.get(";"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};
    vm.run(root_fn);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    0 : 42\n"
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

    auto root_fn = gc_heap.make<motts::lox::Function>({});

    root_fn->chunk.emit_constant(1.0, Token{interned_strings.get("1"), 1});
    root_fn->chunk.emit_constant(2.0, Token{interned_strings.get("2"), 1});
    root_fn->chunk.emit<Opcode::greater>(Token{interned_strings.get(">"), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(3.0, Token{interned_strings.get("3"), 1});
    root_fn->chunk.emit_constant(5.0, Token{interned_strings.get("5"), 1});
    root_fn->chunk.emit<Opcode::less>(Token{interned_strings.get(">="), 1});
    root_fn->chunk.emit<Opcode::not_>(Token{interned_strings.get(">="), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(7.0, Token{interned_strings.get("7"), 1});
    root_fn->chunk.emit_constant(11.0, Token{interned_strings.get("11"), 1});
    root_fn->chunk.emit<Opcode::equal>(Token{interned_strings.get("=="), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(13.0, Token{interned_strings.get("13"), 1});
    root_fn->chunk.emit_constant(17.0, Token{interned_strings.get("17"), 1});
    root_fn->chunk.emit<Opcode::equal>(Token{interned_strings.get("!="), 1});
    root_fn->chunk.emit<Opcode::not_>(Token{interned_strings.get("!="), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(19.0, Token{interned_strings.get("19"), 1});
    root_fn->chunk.emit_constant(23.0, Token{interned_strings.get("23"), 1});
    root_fn->chunk.emit<Opcode::greater>(Token{interned_strings.get("<="), 1});
    root_fn->chunk.emit<Opcode::not_>(Token{interned_strings.get("<="), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(29.0, Token{interned_strings.get("29"), 1});
    root_fn->chunk.emit_constant(31.0, Token{interned_strings.get("31"), 1});
    root_fn->chunk.emit<Opcode::less>(Token{interned_strings.get("<"), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(interned_strings.get("42"), Token{interned_strings.get("\"42\""), 1});
    root_fn->chunk.emit_constant(interned_strings.get("42"), Token{interned_strings.get("\"42\""), 1});
    root_fn->chunk.emit<Opcode::equal>(Token{interned_strings.get("=="), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    root_fn->chunk.emit_constant(interned_strings.get("42"), Token{interned_strings.get("\"42\""), 1});
    root_fn->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    root_fn->chunk.emit<Opcode::equal>(Token{interned_strings.get("=="), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(root_fn);

    BOOST_TEST(os.str() == "false\nfalse\nfalse\ntrue\ntrue\ntrue\ntrue\nfalse\n");
}

BOOST_AUTO_TEST_CASE(invalid_comparisons_will_throw)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    {
        auto root_fn = gc_heap.make<motts::lox::Function>({});
        root_fn->chunk.emit_constant(1.0, Token{interned_strings.get("1"), 1});
        root_fn->chunk.emit_constant(true, Token{interned_strings.get("true"), 1});
        root_fn->chunk.emit<Opcode::greater>(Token{interned_strings.get(">"), 1});

        BOOST_CHECK_THROW(vm.run(root_fn), std::runtime_error);
        try {
            vm.run(root_fn);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \">\": Operands must be numbers.");
        }
    }
    {
        auto root_fn = gc_heap.make<motts::lox::Function>({});
        root_fn->chunk.emit_constant(interned_strings.get("hello"), Token{interned_strings.get("\"hello\""), 1});
        root_fn->chunk.emit_constant(1.0, Token{interned_strings.get("1"), 1});
        root_fn->chunk.emit<Opcode::less>(Token{interned_strings.get("<"), 1});

        BOOST_CHECK_THROW(vm.run(root_fn), std::runtime_error);
        try {
            vm.run(root_fn);
        } catch (const std::exception& error) {
            BOOST_TEST(error.what() == "[Line 1] Error at \"<\": Operands must be numbers.");
        }
    }
}

BOOST_AUTO_TEST_CASE(jump_if_false_will_run)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    auto root_fn = gc_heap.make<motts::lox::Function>({});

    root_fn->chunk.emit<Opcode::true_>(Token{interned_strings.get("true"), 1});
    {
        auto jump_backpatch = root_fn->chunk.emit_jump_if_false(Token{interned_strings.get("and"), 1});
        root_fn->chunk.emit_constant(interned_strings.get("if true"), Token{interned_strings.get("\"if true\""), 1});
        root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
        jump_backpatch.to_next_opcode();

        root_fn->chunk.emit_constant(interned_strings.get("if end"), Token{interned_strings.get("\"if end\""), 1});
        root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
    }

    root_fn->chunk.emit<Opcode::false_>(Token{interned_strings.get("false"), 1});
    {
        auto jump_backpatch = root_fn->chunk.emit_jump_if_false(Token{interned_strings.get("and"), 1});
        root_fn->chunk.emit_constant(interned_strings.get("if true"), Token{interned_strings.get("\"if true\""), 1});
        root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
        jump_backpatch.to_next_opcode();

        root_fn->chunk.emit_constant(interned_strings.get("if end"), Token{interned_strings.get("\"if end\""), 1});
        root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
    }

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(root_fn);

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

    auto root_fn = gc_heap.make<motts::lox::Function>({});

    auto jump_backpatch = root_fn->chunk.emit_jump(Token{interned_strings.get("or"), 1});
    root_fn->chunk.emit_constant(interned_strings.get("skip"), Token{interned_strings.get("\"skip\""), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
    jump_backpatch.to_next_opcode();

    root_fn->chunk.emit_constant(interned_strings.get("end"), Token{interned_strings.get("\"end\""), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(root_fn);

    BOOST_TEST(os.str() == "end\n");
}

BOOST_AUTO_TEST_CASE(if_else_will_leave_clean_stack)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};

    vm.run(compile(gc_heap, interned_strings, "if (true) nil;"));

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n\n"
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; true @ 1\n"
        "    1 : 1a 00 06 JUMP_IF_FALSE +6 -> 10  ; if @ 1\n"
        "    4 : 04       POP                     ; if @ 1\n"
        "    5 : 01       NIL                     ; nil @ 1\n"
        "    6 : 04       POP                     ; ; @ 1\n"
        "    7 : 19 00 01 JUMP +1 -> 11           ; if @ 1\n"
        "   10 : 04       POP                     ; if @ 1\n"
        "Constants:\n"
        "    -\n"
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

    auto root_fn = gc_heap.make<motts::lox::Function>({});
    root_fn->chunk.emit<Opcode::nil>(Token{interned_strings.get("var"), 1});
    root_fn->chunk.emit<Opcode::define_global>(interned_strings.get("x"), Token{interned_strings.get("var"), 1});
    root_fn->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    root_fn->chunk.emit<Opcode::set_global>(interned_strings.get("x"), Token{interned_strings.get("x"), 1});
    root_fn->chunk.emit<Opcode::pop>(Token{interned_strings.get(";"), 1});
    root_fn->chunk.emit<Opcode::get_global>(interned_strings.get("x"), Token{interned_strings.get("x"), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(root_fn);

    BOOST_TEST(os.str() == "42\n");
}

BOOST_AUTO_TEST_CASE(get_global_of_undeclared_will_throw)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    auto root_fn = gc_heap.make<motts::lox::Function>({});
    root_fn->chunk.emit<Opcode::get_global>(interned_strings.get("x"), Token{interned_strings.get("x"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};

    BOOST_CHECK_THROW(vm.run(root_fn), std::runtime_error);
    try {
        vm.run(root_fn);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error: Undefined variable \"x\".");
    }
}

BOOST_AUTO_TEST_CASE(set_global_of_undeclared_will_throw)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    auto root_fn = gc_heap.make<motts::lox::Function>({});
    root_fn->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    root_fn->chunk.emit<Opcode::set_global>(interned_strings.get("x"), Token{interned_strings.get("x"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};

    BOOST_CHECK_THROW(vm.run(root_fn), std::runtime_error);
    try {
        vm.run(root_fn);
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
        auto root_fn = gc_heap.make<motts::lox::Function>({});
        root_fn->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
        root_fn->chunk.emit<Opcode::define_global>(interned_strings.get("x"), Token{interned_strings.get("var"), 1});

        vm.run(root_fn);
    }
    {
        auto root_fn = gc_heap.make<motts::lox::Function>({});
        root_fn->chunk.emit<Opcode::get_global>(interned_strings.get("x"), Token{interned_strings.get("x"), 1});
        root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

        vm.run(root_fn);
    }

    BOOST_TEST(os.str() == "42\n");
}

BOOST_AUTO_TEST_CASE(loop_will_run)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    auto root_fn = gc_heap.make<motts::lox::Function>({});

    root_fn->chunk.emit_constant(true, Token{interned_strings.get("true"), 1});
    const auto condition_begin_bytecode_index = root_fn->chunk.bytecode().size();
    auto jump_backpatch = root_fn->chunk.emit_jump_if_false(Token{interned_strings.get("while"), 1});

    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
    root_fn->chunk.emit_constant(false, Token{interned_strings.get("false"), 1});
    root_fn->chunk.emit_loop(condition_begin_bytecode_index, Token{interned_strings.get("while"), 1});

    jump_backpatch.to_next_opcode();
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(root_fn);

    BOOST_TEST(os.str() == "true\nfalse\n");
}

BOOST_AUTO_TEST_CASE(global_var_declaration_will_run)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    auto root_fn = gc_heap.make<motts::lox::Function>({});
    root_fn->chunk.emit<Opcode::nil>(Token{interned_strings.get("var"), 1});
    root_fn->chunk.emit<Opcode::define_global>(interned_strings.get("x"), Token{interned_strings.get("var"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};
    vm.run(root_fn);

    BOOST_TEST(os.str() == "");
}

BOOST_AUTO_TEST_CASE(global_var_will_initialize_from_stack)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    auto root_fn = gc_heap.make<motts::lox::Function>({});
    root_fn->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    root_fn->chunk.emit<Opcode::define_global>(interned_strings.get("x"), Token{interned_strings.get("var"), 1});
    root_fn->chunk.emit<Opcode::get_global>(interned_strings.get("x"), Token{interned_strings.get("x"), 1});
    root_fn->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};
    vm.run(root_fn);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 08 01    DEFINE_GLOBAL [1]       ; var @ 1\n"
        "    4 : 07 01    GET_GLOBAL [1]          ; x @ 1\n"
        "    6 : 18       PRINT                   ; print @ 1\n"
        "Constants:\n"
        "    0 : 42\n"
        "    1 : x\n"
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

    auto root_fn = gc_heap.make<motts::lox::Function>({});
    root_fn->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    root_fn->chunk.emit<Opcode::get_local>(0, Token{interned_strings.get("x"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};
    vm.run(root_fn);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 05 00    GET_LOCAL [0]           ; x @ 1\n"
        "Constants:\n"
        "    0 : 42\n"
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

    auto root_fn = gc_heap.make<motts::lox::Function>({});
    root_fn->chunk.emit_constant(28.0, Token{interned_strings.get("28"), 1});
    root_fn->chunk.emit_constant(14.0, Token{interned_strings.get("14"), 1});
    root_fn->chunk.emit<Opcode::set_local>(0, Token{interned_strings.get("x"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os, /* debug = */ true};
    vm.run(root_fn);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 28 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 14 @ 1\n"
        "    4 : 06 00    SET_LOCAL [0]           ; x @ 1\n"
        "Constants:\n"
        "    0 : 28\n"
        "    1 : 14\n"
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

    auto fn_f = gc_heap.make<motts::lox::Function>({interned_strings.get("f"), 1, {}});
    fn_f->chunk.emit<Opcode::get_local>(0, Token{interned_strings.get("f"), 1});
    fn_f->chunk.emit<Opcode::get_local>(1, Token{interned_strings.get("x"), 1});

    auto fn_main = gc_heap.make<motts::lox::Function>({});
    fn_main->chunk.emit_closure(fn_f, {}, Token{interned_strings.get("fun"), 1});
    fn_main->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    fn_main->chunk.emit_call(1, Token{interned_strings.get("f"), 1});

    vm.run(fn_main);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n"
        "\n"
        "Bytecode:\n"
        "    0 : 1f 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 00 01    CONSTANT [1]            ; 42 @ 1\n"
        "    5 : 1c 01    CALL (1)                ; f @ 1\n"
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : 42\n"
        "[<fn f> chunk]\n"
        "Bytecode:\n"
        "    0 : 05 00    GET_LOCAL [0]           ; f @ 1\n"
        "    2 : 05 01    GET_LOCAL [1]           ; x @ 1\n"
        "Constants:\n"
        "    -\n"
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

    auto fn_f = gc_heap.make<motts::lox::Function>({interned_strings.get("f"), 1, {}});
    fn_f->chunk.emit<Opcode::get_local>(0, Token{interned_strings.get("f"), 1});
    fn_f->chunk.emit<Opcode::get_local>(1, Token{interned_strings.get("x"), 1});
    fn_f->chunk.emit<Opcode::return_>(Token{interned_strings.get("return"), 1});

    auto fn_main = gc_heap.make<motts::lox::Function>({});
    fn_main->chunk.emit<Opcode::nil>(Token{interned_strings.get("nil"), 1}); // Force the call frame's stack offset to matter.
    fn_main->chunk.emit_closure(fn_f, {}, Token{interned_strings.get("fun"), 1});
    fn_main->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    fn_main->chunk.emit_call(1, Token{interned_strings.get("f"), 1});
    fn_main->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    vm.run(fn_main);

    // clang-format off
    const auto expected =
        "\n# Running chunk:\n"
        "\n"
        "Bytecode:\n"
        "    0 : 01       NIL                     ; nil @ 1\n"
        "    1 : 1f 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    4 : 00 01    CONSTANT [1]            ; 42 @ 1\n"
        "    6 : 1c 01    CALL (1)                ; f @ 1\n"
        "    8 : 18       PRINT                   ; print @ 1\n"
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : 42\n"
        "[<fn f> chunk]\n"
        "Bytecode:\n"
        "    0 : 05 00    GET_LOCAL [0]           ; f @ 1\n"
        "    2 : 05 01    GET_LOCAL [1]           ; x @ 1\n"
        "    4 : 21       RETURN                  ; return @ 1\n"
        "Constants:\n"
        "    -\n"
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

    auto fn_f = gc_heap.make<motts::lox::Function>({interned_strings.get("f"), 1, {}});
    fn_f->chunk.emit<Opcode::nil>(Token{interned_strings.get("fun"), 1});
    fn_f->chunk.emit<Opcode::return_>(Token{interned_strings.get("fun"), 1});

    auto fn_main = gc_heap.make<motts::lox::Function>({});
    fn_main->chunk.emit_closure(fn_f, {}, Token{interned_strings.get("fun"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("f"), 1});

    BOOST_CHECK_THROW(vm.run(fn_main), std::runtime_error);
    try {
        vm.run(fn_main);
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"f\": Expected 1 arguments but got 0.");
    }
}

BOOST_AUTO_TEST_CASE(calling_a_noncallable_will_throw)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    auto root_fn = gc_heap.make<motts::lox::Function>({});
    root_fn->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    root_fn->chunk.emit_call(0, Token{interned_strings.get("42"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};

    BOOST_CHECK_THROW(vm.run(root_fn), std::runtime_error);
    try {
        vm.run(root_fn);
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

    auto fn_inner_get = gc_heap.make<motts::lox::Function>({});
    fn_inner_get->chunk.emit<Opcode::get_upvalue>(0, Token{interned_strings.get("x"), 1});
    fn_inner_get->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
    fn_inner_get->chunk.emit<Opcode::return_>(Token{interned_strings.get("return"), 1});

    auto fn_inner_set = gc_heap.make<motts::lox::Function>({});
    fn_inner_set->chunk.emit_constant(14.0, Token{interned_strings.get("14"), 1});
    fn_inner_set->chunk.emit<Opcode::set_upvalue>(0, Token{interned_strings.get("x"), 1});
    fn_inner_set->chunk.emit<Opcode::return_>(Token{interned_strings.get("return"), 1});

    auto fn_middle = gc_heap.make<motts::lox::Function>({});
    fn_middle->chunk.emit_closure(fn_inner_get, {motts::lox::UpUpvalue_index{0}}, Token{interned_strings.get("fun"), 1});
    fn_middle->chunk.emit<Opcode::set_global>(interned_strings.get("get"), Token{interned_strings.get("get"), 1});
    fn_middle->chunk.emit<Opcode::pop>(Token{interned_strings.get(";"), 1});
    fn_middle->chunk.emit_closure(fn_inner_set, {motts::lox::UpUpvalue_index{0}}, Token{interned_strings.get("fun"), 1});
    fn_middle->chunk.emit<Opcode::set_global>(interned_strings.get("set"), Token{interned_strings.get("set"), 1});
    fn_middle->chunk.emit<Opcode::pop>(Token{interned_strings.get(";"), 1});
    fn_middle->chunk.emit<Opcode::return_>(Token{interned_strings.get("return"), 1});

    auto fn_outer = gc_heap.make<motts::lox::Function>({});
    fn_outer->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    fn_outer->chunk.emit_closure(fn_middle, {motts::lox::Upvalue_index{1}}, Token{interned_strings.get("fun"), 1});
    fn_outer->chunk.emit_call(0, Token{interned_strings.get("middle"), 1});
    fn_outer->chunk.emit<Opcode::pop>(Token{interned_strings.get(";"), 1});
    fn_outer->chunk.emit<Opcode::close_upvalue>(Token{interned_strings.get("}"), 1});
    fn_outer->chunk.emit<Opcode::return_>(Token{interned_strings.get("return"), 1});

    auto fn_main = gc_heap.make<motts::lox::Function>({});
    fn_main->chunk.emit<Opcode::nil>(Token{interned_strings.get("var"), 1});
    fn_main->chunk.emit<Opcode::define_global>(interned_strings.get("get"), Token{interned_strings.get("var"), 1});
    fn_main->chunk.emit<Opcode::nil>(Token{interned_strings.get("var"), 1});
    fn_main->chunk.emit<Opcode::define_global>(interned_strings.get("set"), Token{interned_strings.get("var"), 1});
    fn_main->chunk.emit_closure(fn_outer, {}, Token{interned_strings.get("fun"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("outer"), 1});
    fn_main->chunk.emit<Opcode::get_global>(interned_strings.get("get"), Token{interned_strings.get("get"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("get"), 1});
    fn_main->chunk.emit<Opcode::get_global>(interned_strings.get("set"), Token{interned_strings.get("set"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("set"), 1});
    fn_main->chunk.emit<Opcode::get_global>(interned_strings.get("get"), Token{interned_strings.get("get"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("get"), 1});

    vm.run(fn_main);

    BOOST_TEST(os.str() == "42\n14\n");
}

BOOST_AUTO_TEST_CASE(closure_decl_and_capture_can_be_out_of_order)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    auto fn_inner_get = gc_heap.make<motts::lox::Function>({});
    fn_inner_get->chunk.emit<Opcode::get_upvalue>(0, Token{interned_strings.get("x"), 1});
    fn_inner_get->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
    fn_inner_get->chunk.emit<Opcode::get_upvalue>(1, Token{interned_strings.get("y"), 1});
    fn_inner_get->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
    fn_inner_get->chunk.emit<Opcode::return_>(Token{interned_strings.get("return"), 1});

    auto fn_outer = gc_heap.make<motts::lox::Function>({});
    fn_outer->chunk.emit<Opcode::nil>(Token{interned_strings.get("var"), 1});
    fn_outer->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    fn_outer->chunk.emit_constant(14.0, Token{interned_strings.get("42"), 1});
    fn_outer->chunk
        .emit_closure(fn_inner_get, {motts::lox::Upvalue_index{3}, motts::lox::Upvalue_index{2}}, Token{interned_strings.get("fun"), 1});
    fn_outer->chunk.emit<Opcode::set_local>(1, Token{interned_strings.get("closure"), 1});
    fn_outer->chunk.emit<Opcode::pop>(Token{interned_strings.get("}"), 1});
    fn_outer->chunk.emit<Opcode::close_upvalue>(Token{interned_strings.get("}"), 1});
    fn_outer->chunk.emit<Opcode::close_upvalue>(Token{interned_strings.get("}"), 1});
    fn_outer->chunk.emit_call(0, Token{interned_strings.get("get"), 1});
    fn_outer->chunk.emit<Opcode::return_>(Token{interned_strings.get("return"), 1});

    auto fn_main = gc_heap.make<motts::lox::Function>({});
    fn_main->chunk.emit_closure(fn_outer, {}, Token{interned_strings.get("fun"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("outer"), 1});

    vm.run(fn_main);

    BOOST_TEST(os.str() == "14\n42\n");
}

BOOST_AUTO_TEST_CASE(closure_early_return_will_close_upvalues)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    auto fn_inner_get = gc_heap.make<motts::lox::Function>({});
    fn_inner_get->chunk.emit<Opcode::get_upvalue>(0, Token{interned_strings.get("x"), 1});
    fn_inner_get->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
    fn_inner_get->chunk.emit<Opcode::return_>(Token{interned_strings.get("return"), 1});

    auto fn_outer = gc_heap.make<motts::lox::Function>({});
    fn_outer->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    fn_outer->chunk.emit_closure(fn_inner_get, {motts::lox::Upvalue_index{1}}, Token{interned_strings.get("fun"), 1});
    fn_outer->chunk.emit<Opcode::return_>(Token{interned_strings.get("return"), 1});

    auto fn_main = gc_heap.make<motts::lox::Function>({});
    fn_main->chunk.emit_closure(fn_outer, {}, Token{interned_strings.get("fun"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("outer"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("get"), 1});

    vm.run(fn_main);

    BOOST_TEST(os.str() == "42\n");
}

BOOST_AUTO_TEST_CASE(class_will_run)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    auto fn_method = gc_heap.make<motts::lox::Function>({});
    fn_method->chunk.emit<Opcode::nil>(Token{interned_strings.get("method"), 1});
    fn_method->chunk.emit<Opcode::return_>(Token{interned_strings.get("return"), 1});

    auto fn_main = gc_heap.make<motts::lox::Function>({});
    fn_main->chunk.emit<Opcode::class_>(interned_strings.get("Klass"), Token{interned_strings.get("class"), 1});
    fn_main->chunk.emit_closure(fn_method, {}, Token{interned_strings.get("method"), 1});
    fn_main->chunk.emit<Opcode::method>(interned_strings.get("method"), Token{interned_strings.get("method"), 1});
    fn_main->chunk.emit<Opcode::define_global>(interned_strings.get("Klass"), Token{interned_strings.get("class"), 1});

    fn_main->chunk.emit<Opcode::get_global>(interned_strings.get("Klass"), Token{interned_strings.get("Klass"), 1});
    fn_main->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    fn_main->chunk.emit<Opcode::get_global>(interned_strings.get("Klass"), Token{interned_strings.get("Klass"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("Klass"), 1});
    fn_main->chunk.emit<Opcode::define_global>(interned_strings.get("instance"), Token{interned_strings.get("var"), 1});

    fn_main->chunk.emit<Opcode::get_global>(interned_strings.get("instance"), Token{interned_strings.get("instance"), 1});
    fn_main->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    fn_main->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    fn_main->chunk.emit<Opcode::get_global>(interned_strings.get("instance"), Token{interned_strings.get("instance"), 1});
    fn_main->chunk.emit<Opcode::set_property>(interned_strings.get("property"), Token{interned_strings.get("property"), 1});
    fn_main->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    fn_main->chunk.emit<Opcode::get_global>(interned_strings.get("instance"), Token{interned_strings.get("instance"), 1});
    fn_main->chunk.emit<Opcode::get_property>(interned_strings.get("property"), Token{interned_strings.get("property"), 1});
    fn_main->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    vm.run(fn_main);

    BOOST_TEST(os.str() == "<class Klass>\n<instance Klass>\n42\n42\n");
}

BOOST_AUTO_TEST_CASE(methods_bind_and_can_be_called)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    auto fn_method = gc_heap.make<motts::lox::Function>({interned_strings.get("method"), 0, {}});
    fn_method->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    fn_method->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
    fn_method->chunk.emit<Opcode::nil>(Token{interned_strings.get("method"), 1});
    fn_method->chunk.emit<Opcode::return_>(Token{interned_strings.get("return"), 1});

    auto fn_main = gc_heap.make<motts::lox::Function>({});
    fn_main->chunk.emit<Opcode::class_>(interned_strings.get("Klass"), Token{interned_strings.get("class"), 1});
    fn_main->chunk.emit_closure(fn_method, {}, Token{interned_strings.get("method"), 1});
    fn_main->chunk.emit<Opcode::method>(interned_strings.get("method"), Token{interned_strings.get("method"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("Klass"), 1});
    fn_main->chunk.emit<Opcode::get_local>(0, Token{interned_strings.get("instance"), 1});
    fn_main->chunk.emit<Opcode::get_property>(interned_strings.get("method"), Token{interned_strings.get("method"), 1});
    fn_main->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
    fn_main->chunk.emit<Opcode::get_local>(0, Token{interned_strings.get("instance"), 1});
    fn_main->chunk.emit<Opcode::get_property>(interned_strings.get("method"), Token{interned_strings.get("method"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("method"), 1});

    vm.run(fn_main);

    BOOST_TEST(os.str() == "<fn method>\n42\n");
}

BOOST_AUTO_TEST_CASE(this_can_be_captured_in_closure)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    auto fn_inner = gc_heap.make<motts::lox::Function>({});
    fn_inner->chunk.emit<Opcode::get_upvalue>(0, Token{interned_strings.get("this"), 1});
    fn_inner->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
    fn_inner->chunk.emit<Opcode::nil>(Token{interned_strings.get("method"), 1});
    fn_inner->chunk.emit<Opcode::return_>(Token{interned_strings.get("return"), 1});

    auto fn_method = gc_heap.make<motts::lox::Function>({});
    fn_method->chunk.emit_closure(fn_inner, {motts::lox::Upvalue_index{0}}, Token{interned_strings.get("fun"), 1});
    fn_method->chunk.emit<Opcode::return_>(Token{interned_strings.get("return"), 1});

    auto fn_main = gc_heap.make<motts::lox::Function>({});
    fn_main->chunk.emit<Opcode::class_>(interned_strings.get("Klass"), Token{interned_strings.get("class"), 1});
    fn_main->chunk.emit_closure(fn_method, {}, Token{interned_strings.get("method"), 1});
    fn_main->chunk.emit<Opcode::method>(interned_strings.get("method"), Token{interned_strings.get("method"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("Klass"), 1});
    fn_main->chunk.emit<Opcode::get_property>(interned_strings.get("method"), Token{interned_strings.get("method"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("method"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("method"), 1});

    vm.run(fn_main);

    BOOST_TEST(os.str() == "<instance Klass>\n");
}

BOOST_AUTO_TEST_CASE(init_method_will_run_when_instance_created)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    auto fn_init = gc_heap.make<motts::lox::Function>({});
    fn_init->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    fn_init->chunk.emit<Opcode::get_local>(0, Token{interned_strings.get("this"), 1});
    fn_init->chunk.emit<Opcode::set_property>(interned_strings.get("property"), Token{interned_strings.get("property"), 1});
    fn_init->chunk.emit<Opcode::get_local>(0, Token{interned_strings.get("this"), 1});
    fn_init->chunk.emit<Opcode::return_>(Token{interned_strings.get("return"), 1});

    auto fn_main = gc_heap.make<motts::lox::Function>({});
    fn_main->chunk.emit<Opcode::class_>(interned_strings.get("Klass"), Token{interned_strings.get("class"), 1});
    fn_main->chunk.emit_closure(fn_init, {}, Token{interned_strings.get("init"), 1});
    fn_main->chunk.emit<Opcode::method>(interned_strings.get("init"), Token{interned_strings.get("init"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("Klass"), 1});
    fn_main->chunk.emit<Opcode::get_property>(interned_strings.get("property"), Token{interned_strings.get("property"), 1});
    fn_main->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    vm.run(fn_main);

    BOOST_TEST(os.str() == "42\n");
}

BOOST_AUTO_TEST_CASE(class_methods_can_inherit)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    auto parent_method = gc_heap.make<motts::lox::Function>({});
    parent_method->chunk.emit_constant(interned_strings.get("Parent"), Token{interned_strings.get("\"Parent\""), 1});
    parent_method->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
    parent_method->chunk.emit<Opcode::nil>(Token{interned_strings.get("parentMethod"), 1});
    parent_method->chunk.emit<Opcode::return_>(Token{interned_strings.get("parentMethod"), 1});

    auto child_method = gc_heap.make<motts::lox::Function>({});
    child_method->chunk.emit_constant(interned_strings.get("Child"), Token{interned_strings.get("\"Child\""), 1});
    child_method->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
    child_method->chunk.emit<Opcode::nil>(Token{interned_strings.get("childMethod"), 1});
    child_method->chunk.emit<Opcode::return_>(Token{interned_strings.get("childMethod"), 1});

    auto fn_main = gc_heap.make<motts::lox::Function>({});
    fn_main->chunk.emit<Opcode::class_>(interned_strings.get("Parent"), Token{interned_strings.get("class"), 1});
    fn_main->chunk.emit_closure(parent_method, {}, Token{interned_strings.get("parentMethod1"), 1});
    fn_main->chunk.emit<Opcode::method>(interned_strings.get("parentMethod1"), Token{interned_strings.get("parentMethod1"), 1});
    fn_main->chunk.emit_closure(parent_method, {}, Token{interned_strings.get("parentMethod2"), 1});
    fn_main->chunk.emit<Opcode::method>(interned_strings.get("parentMethod2"), Token{interned_strings.get("parentMethod2"), 1});

    fn_main->chunk.emit<Opcode::class_>(interned_strings.get("Child"), Token{interned_strings.get("class"), 1});
    fn_main->chunk.emit<Opcode::get_local>(0, Token{interned_strings.get("Parent"), 1});
    fn_main->chunk.emit<Opcode::inherit>(Token{interned_strings.get("Parent"), 1});
    fn_main->chunk.emit_closure(child_method, {}, Token{interned_strings.get("childMethod"), 1});
    fn_main->chunk.emit<Opcode::method>(interned_strings.get("childMethod"), Token{interned_strings.get("childMethod"), 1});
    fn_main->chunk.emit_closure(child_method, {}, Token{interned_strings.get("parentMethod2"), 1});
    fn_main->chunk.emit<Opcode::method>(interned_strings.get("parentMethod2"), Token{interned_strings.get("parentMethod2"), 1});
    fn_main->chunk.emit<Opcode::pop>(Token{interned_strings.get("Parent"), 1});
    fn_main->chunk.emit<Opcode::pop>(Token{interned_strings.get("Parent"), 1});

    fn_main->chunk.emit_call(0, Token{interned_strings.get("Child"), 1});
    fn_main->chunk.emit<Opcode::get_local>(1, Token{interned_strings.get("instance"), 1});
    fn_main->chunk.emit<Opcode::get_property>(interned_strings.get("childMethod"), Token{interned_strings.get("childMethod"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("childMethod"), 1});
    fn_main->chunk.emit<Opcode::get_local>(1, Token{interned_strings.get("instance"), 1});
    fn_main->chunk.emit<Opcode::get_property>(interned_strings.get("parentMethod1"), Token{interned_strings.get("parentMethod1"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("parentMethod1"), 1});
    fn_main->chunk.emit<Opcode::get_local>(1, Token{interned_strings.get("instance"), 1});
    fn_main->chunk.emit<Opcode::get_property>(interned_strings.get("parentMethod2"), Token{interned_strings.get("parentMethod2"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("parentMethod2"), 1});

    vm.run(fn_main);

    BOOST_TEST(os.str() == "Child\nParent\nChild\n");
}

BOOST_AUTO_TEST_CASE(inheriting_from_a_non_class_will_throw)
{
    std::ostringstream os;
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    motts::lox::VM vm{gc_heap, interned_strings, os};

    auto root_fn = gc_heap.make<motts::lox::Function>({});
    root_fn->chunk.emit<Opcode::class_>(interned_strings.get("Klass"), Token{interned_strings.get("class"), 1});
    root_fn->chunk.emit_constant(42.0, Token{interned_strings.get("42"), 1});
    root_fn->chunk.emit<Opcode::inherit>(Token{interned_strings.get("42"), 1});

    BOOST_CHECK_THROW(vm.run(root_fn), std::runtime_error);
    try {
        vm.run(root_fn);
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

    auto parent_method = gc_heap.make<motts::lox::Function>({});
    parent_method->chunk.emit_constant(interned_strings.get("Parent"), Token{interned_strings.get("\"Parent\""), 1});
    parent_method->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
    parent_method->chunk.emit<Opcode::nil>(Token{interned_strings.get("parentMethod"), 1});
    parent_method->chunk.emit<Opcode::return_>(Token{interned_strings.get("parentMethod"), 1});

    auto child_method = gc_heap.make<motts::lox::Function>({});
    child_method->chunk.emit_constant(interned_strings.get("Child"), Token{interned_strings.get("\"Child\""), 1});
    child_method->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});
    child_method->chunk.emit<Opcode::get_upvalue>(0, Token{interned_strings.get("super"), 1});
    child_method->chunk.emit<Opcode::get_super>(interned_strings.get("method"), Token{interned_strings.get("method"), 1});
    child_method->chunk.emit_call(0, Token{interned_strings.get("method"), 1});
    child_method->chunk.emit<Opcode::nil>(Token{interned_strings.get("childMethod"), 1});
    child_method->chunk.emit<Opcode::return_>(Token{interned_strings.get("childMethod"), 1});

    auto fn_main = gc_heap.make<motts::lox::Function>({});
    fn_main->chunk.emit<Opcode::class_>(interned_strings.get("Parent"), Token{interned_strings.get("class"), 1});
    fn_main->chunk.emit_closure(parent_method, {}, Token{interned_strings.get("method"), 1});
    fn_main->chunk.emit<Opcode::method>(interned_strings.get("method"), Token{interned_strings.get("method"), 1});

    fn_main->chunk.emit<Opcode::class_>(interned_strings.get("Child"), Token{interned_strings.get("class"), 1});
    fn_main->chunk.emit<Opcode::get_local>(0, Token{interned_strings.get("Parent"), 1});
    fn_main->chunk.emit<Opcode::inherit>(Token{interned_strings.get("Parent"), 1});
    fn_main->chunk.emit_closure(child_method, {motts::lox::Upvalue_index{2}}, Token{interned_strings.get("method"), 1});
    fn_main->chunk.emit<Opcode::method>(interned_strings.get("method"), Token{interned_strings.get("method"), 1});
    fn_main->chunk.emit<Opcode::pop>(Token{interned_strings.get("Parent"), 1});
    fn_main->chunk.emit<Opcode::close_upvalue>(Token{interned_strings.get("Parent"), 1});

    fn_main->chunk.emit_call(0, Token{interned_strings.get("Child"), 1});
    fn_main->chunk.emit<Opcode::get_property>(interned_strings.get("method"), Token{interned_strings.get("method"), 1});
    fn_main->chunk.emit_call(0, Token{interned_strings.get("method"), 1});

    vm.run(fn_main);

    BOOST_TEST(os.str() == "Child\nParent\n");
}

BOOST_AUTO_TEST_CASE(native_clock_fn_will_run)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    auto fn_now = gc_heap.make<motts::lox::Function>({});
    fn_now->chunk.emit<Opcode::get_global>(interned_strings.get("clock"), Token{interned_strings.get("clock"), 1});
    fn_now->chunk.emit_call(0, Token{interned_strings.get("clock"), 1});
    fn_now->chunk.emit<Opcode::define_global>(interned_strings.get("now"), Token{interned_strings.get("var"), 1});

    auto fn_later = gc_heap.make<motts::lox::Function>({});
    fn_later->chunk.emit<Opcode::get_global>(interned_strings.get("clock"), Token{interned_strings.get("clock"), 1});
    fn_later->chunk.emit_call(0, Token{interned_strings.get("clock"), 1});
    fn_later->chunk.emit<Opcode::define_global>(interned_strings.get("later"), Token{interned_strings.get("var"), 1});

    fn_later->chunk.emit<Opcode::get_global>(interned_strings.get("later"), Token{interned_strings.get("later"), 1});
    fn_later->chunk.emit<Opcode::get_global>(interned_strings.get("now"), Token{interned_strings.get("now"), 1});
    fn_later->chunk.emit<Opcode::greater>(Token{interned_strings.get(">"), 1});
    fn_later->chunk.emit<Opcode::print>(Token{interned_strings.get("print"), 1});

    std::ostringstream os;
    motts::lox::VM vm{gc_heap, interned_strings, os};

    vm.run(fn_now);
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
    }
    vm.run(fn_later);

    BOOST_TEST(os.str() == "true\n");
}
