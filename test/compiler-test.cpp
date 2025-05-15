#define BOOST_TEST_MODULE Compiler Tests

#include <sstream>
#include <stdexcept>

#include <boost/test/unit_test.hpp>

#include "../src/compiler.hpp"
#include "../src/interned_strings.hpp"
#include "../src/object.hpp"

BOOST_AUTO_TEST_CASE(opcodes_can_be_printed)
{
    std::ostringstream os;
    os << motts::lox::Opcode::constant;
    BOOST_TEST(os.str() == "CONSTANT");
}

BOOST_AUTO_TEST_CASE(printing_invalid_opcode_will_throw)
{
    union Invalid_opcode
    {
        motts::lox::Opcode as_opcode;
        int as_int;
    };

    Invalid_opcode invalid_opcode;
    invalid_opcode.as_int = 276'709; // Don't panic.

    std::ostringstream os;
    BOOST_CHECK_THROW(os << invalid_opcode.as_opcode, std::logic_error);
}

BOOST_AUTO_TEST_CASE(chunks_can_be_printed)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    motts::lox::Chunk chunk;
    chunk.emit_constant(42.0, motts::lox::Source_map_token{interned_strings.get("42"), 1});
    chunk.emit<motts::lox::Opcode::nil>(motts::lox::Source_map_token{interned_strings.get("nil"), 2});

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 01       NIL                     ; nil @ 2\n"
        "Constants:\n"
        "    0 : 42\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(number_literals_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "42;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    0 : 42\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(nil_literals_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "nil;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 01       NIL                     ; nil @ 1\n"
        "    1 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(invalid_expressions_will_throw)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    BOOST_CHECK_THROW(compile(gc_heap, interned_strings, "?"), std::runtime_error);

    try {
        compile(gc_heap, interned_strings, "?");
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error: Unexpected character \"?\".");
    }
}

BOOST_AUTO_TEST_CASE(true_false_literals_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "true; false;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; true @ 1\n"
        "    1 : 04       POP                     ; ; @ 1\n"
        "    2 : 03       FALSE                   ; false @ 1\n"
        "    3 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(string_literals_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "\"hello\";");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; \"hello\" @ 1\n"
        "    2 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    0 : hello\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(addition_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "28 + 14;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 28 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 14 @ 1\n"
        "    4 : 12       ADD                     ; + @ 1\n"
        "    5 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    0 : 28\n"
        "    1 : 14\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(invalid_addition_will_throw)
{
    const auto expect_to_throw = [] {
        motts::lox::GC_heap gc_heap;
        motts::lox::Interned_strings interned_strings{gc_heap};
        compile(gc_heap, interned_strings, "42 + ");
    };

    BOOST_CHECK_THROW(expect_to_throw(), std::runtime_error);

    try {
        expect_to_throw();
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error: Unexpected token \"EOF\".");
    }
}

BOOST_AUTO_TEST_CASE(print_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "print 42;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 18       PRINT                   ; print @ 1\n"
        "Constants:\n"
        "    0 : 42\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(plus_minus_star_slash_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "1 + 2 - 3 * 5 / 7;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 1 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 2 @ 1\n"
        "    4 : 12       ADD                     ; + @ 1\n"
        "    5 : 00 02    CONSTANT [2]            ; 3 @ 1\n"
        "    7 : 00 03    CONSTANT [3]            ; 5 @ 1\n"
        "    9 : 14       MULTIPLY                ; * @ 1\n"
        "   10 : 00 04    CONSTANT [4]            ; 7 @ 1\n"
        "   12 : 15       DIVIDE                  ; / @ 1\n"
        "   13 : 13       SUBTRACT                ; - @ 1\n"
        "   14 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    0 : 1\n"
        "    1 : 2\n"
        "    2 : 3\n"
        "    3 : 5\n"
        "    4 : 7\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(parens_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "1 + (2 - 3) * 5 / 7;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 1 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 2 @ 1\n"
        "    4 : 00 02    CONSTANT [2]            ; 3 @ 1\n"
        "    6 : 13       SUBTRACT                ; - @ 1\n"
        "    7 : 00 03    CONSTANT [3]            ; 5 @ 1\n"
        "    9 : 14       MULTIPLY                ; * @ 1\n"
        "   10 : 00 04    CONSTANT [4]            ; 7 @ 1\n"
        "   12 : 15       DIVIDE                  ; / @ 1\n"
        "   13 : 12       ADD                     ; + @ 1\n"
        "   14 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    0 : 1\n"
        "    1 : 2\n"
        "    2 : 3\n"
        "    3 : 5\n"
        "    4 : 7\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(numeric_negation_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "-1 + -1;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 1 @ 1\n"
        "    2 : 17       NEGATE                  ; - @ 1\n"
        "    3 : 00 00    CONSTANT [0]            ; 1 @ 1\n"
        "    5 : 17       NEGATE                  ; - @ 1\n"
        "    6 : 12       ADD                     ; + @ 1\n"
        "    7 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    0 : 1\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(boolean_negation_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "!true;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; true @ 1\n"
        "    1 : 16       NOT                     ; ! @ 1\n"
        "    2 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(expression_statements_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "42;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    0 : 42\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(comparisons_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "1 > 2; 3 >= 5; 7 == 11; 13 != 17; 19 <= 23; 29 < 31;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 1 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 2 @ 1\n"
        "    4 : 10       GREATER                 ; > @ 1\n"
        "    5 : 04       POP                     ; ; @ 1\n"

        "    6 : 00 02    CONSTANT [2]            ; 3 @ 1\n"
        "    8 : 00 03    CONSTANT [3]            ; 5 @ 1\n"
        "   10 : 11       LESS                    ; >= @ 1\n"
        "   11 : 16       NOT                     ; >= @ 1\n"
        "   12 : 04       POP                     ; ; @ 1\n"

        "   13 : 00 04    CONSTANT [4]            ; 7 @ 1\n"
        "   15 : 00 05    CONSTANT [5]            ; 11 @ 1\n"
        "   17 : 0f       EQUAL                   ; == @ 1\n"
        "   18 : 04       POP                     ; ; @ 1\n"

        "   19 : 00 06    CONSTANT [6]            ; 13 @ 1\n"
        "   21 : 00 07    CONSTANT [7]            ; 17 @ 1\n"
        "   23 : 0f       EQUAL                   ; != @ 1\n"
        "   24 : 16       NOT                     ; != @ 1\n"
        "   25 : 04       POP                     ; ; @ 1\n"

        "   26 : 00 08    CONSTANT [8]            ; 19 @ 1\n"
        "   28 : 00 09    CONSTANT [9]            ; 23 @ 1\n"
        "   30 : 10       GREATER                 ; <= @ 1\n"
        "   31 : 16       NOT                     ; <= @ 1\n"
        "   32 : 04       POP                     ; ; @ 1\n"

        "   33 : 00 0a    CONSTANT [10]           ; 29 @ 1\n"
        "   35 : 00 0b    CONSTANT [11]           ; 31 @ 1\n"
        "   37 : 11       LESS                    ; < @ 1\n"
        "   38 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    0 : 1\n"
        "    1 : 2\n"
        "    2 : 3\n"
        "    3 : 5\n"
        "    4 : 7\n"
        "    5 : 11\n"
        "    6 : 13\n"
        "    7 : 17\n"
        "    8 : 19\n"
        "    9 : 23\n"
        "   10 : 29\n"
        "   11 : 31\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(boolean_and_with_short_circuit_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "true and false;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; true @ 1\n"
        "    1 : 1a 00 02 JUMP_IF_FALSE +2 -> 6   ; and @ 1\n"
        "    4 : 04       POP                     ; and @ 1\n"
        "    5 : 03       FALSE                   ; false @ 1\n"
        "    6 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(boolean_or_with_short_circuit_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "true or false;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; true @ 1\n"
        "    1 : 1a 00 03 JUMP_IF_FALSE +3 -> 7   ; or @ 1\n"
        "    4 : 19 00 02 JUMP +2 -> 9            ; or @ 1\n"
        "    7 : 04       POP                     ; or @ 1\n"
        "    8 : 03       FALSE                   ; false @ 1\n"
        "    9 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(global_assignment_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "x = 42;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 09 01    SET_GLOBAL [1]          ; x @ 1\n"
        "    4 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    0 : 42\n"
        "    1 : x\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(global_identifier_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "x;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 07 00    GET_GLOBAL [0]          ; x @ 1\n"
        "    2 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    0 : x\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(while_loop_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    const auto loop_expr_fn = compile(gc_heap, interned_strings, "x = 42; while (x > 0) x = x - 1;");
    std::ostringstream os_loop_expr;
    os_loop_expr << loop_expr_fn->chunk;

    const auto loop_block_fn = compile(gc_heap, interned_strings, "x = 42; while (x > 0) { x = x - 1; }");
    std::ostringstream os_loop_block;
    os_loop_block << loop_block_fn->chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 09 01    SET_GLOBAL [1]          ; x @ 1\n"
        "    4 : 04       POP                     ; ; @ 1\n"
        "    5 : 07 01    GET_GLOBAL [1]          ; x @ 1\n"
        "    7 : 00 02    CONSTANT [2]            ; 0 @ 1\n"
        "    9 : 10       GREATER                 ; > @ 1\n"
        "   10 : 1a 00 0c JUMP_IF_FALSE +12 -> 25 ; while @ 1\n"
        "   13 : 04       POP                     ; while @ 1\n"
        "   14 : 07 01    GET_GLOBAL [1]          ; x @ 1\n"
        "   16 : 00 03    CONSTANT [3]            ; 1 @ 1\n"
        "   18 : 13       SUBTRACT                ; - @ 1\n"
        "   19 : 09 01    SET_GLOBAL [1]          ; x @ 1\n"
        "   21 : 04       POP                     ; ; @ 1\n"
        "   22 : 1b 00 14 LOOP -20 -> 5           ; while @ 1\n"
        "   25 : 04       POP                     ; while @ 1\n"
        "Constants:\n"
        "    0 : 42\n"
        "    1 : x\n"
        "    2 : 0\n"
        "    3 : 1\n";
    // clang-format on

    BOOST_TEST(os_loop_expr.str() == expected);
    BOOST_TEST(os_loop_block.str() == expected);
}

BOOST_AUTO_TEST_CASE(blocks_as_statements_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "{ 42; }");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    0 : 42\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(var_declaration_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "var x;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 01       NIL                     ; var @ 1\n"
        "    1 : 08 00    DEFINE_GLOBAL [0]       ; var @ 1\n"
        "Constants:\n"
        "    0 : x\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(var_declarations_can_be_initialized)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "var x = 42;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 08 01    DEFINE_GLOBAL [1]       ; var @ 1\n"
        "Constants:\n"
        "    0 : 42\n"
        "    1 : x\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(vars_will_be_local_inside_braces)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "var x; { var x; x; x = 42; } x;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 01       NIL                     ; var @ 1\n"
        "    1 : 08 00    DEFINE_GLOBAL [0]       ; var @ 1\n"
        "    3 : 01       NIL                     ; var @ 1\n"
        "    4 : 05 00    GET_LOCAL [0]           ; x @ 1\n"
        "    6 : 04       POP                     ; ; @ 1\n"
        "    7 : 00 01    CONSTANT [1]            ; 42 @ 1\n"
        "    9 : 06 00    SET_LOCAL [0]           ; x @ 1\n"
        "   11 : 04       POP                     ; ; @ 1\n"
        "   12 : 04       POP                     ; } @ 1\n"
        "   13 : 07 00    GET_GLOBAL [0]          ; x @ 1\n"
        "   15 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    0 : x\n"
        "    1 : 42\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(redeclared_local_vars_will_throw)
{
    const auto expect_to_throw = [] {
        motts::lox::GC_heap gc_heap;
        motts::lox::Interned_strings interned_strings{gc_heap};
        compile(gc_heap, interned_strings, "{ var x; var x; }");
    };

    BOOST_CHECK_THROW(expect_to_throw(), std::runtime_error);

    try {
        expect_to_throw();
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"x\": Identifier with this name already declared in this scope.");
    }
}

BOOST_AUTO_TEST_CASE(using_local_var_in_own_initializer_will_throw)
{
    const auto expect_to_throw = [] {
        motts::lox::GC_heap gc_heap;
        motts::lox::Interned_strings interned_strings{gc_heap};
        compile(gc_heap, interned_strings, "{ var x = x; }");
    };

    BOOST_CHECK_THROW(expect_to_throw(), std::runtime_error);

    try {
        expect_to_throw();
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"x\": Cannot read local variable in its own initializer.");
    }
}

BOOST_AUTO_TEST_CASE(if_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "if (true) nil;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; true @ 1\n"
        "    1 : 1a 00 06 JUMP_IF_FALSE +6 -> 10  ; if @ 1\n"
        "    4 : 04       POP                     ; if @ 1\n"
        "    5 : 01       NIL                     ; nil @ 1\n"
        "    6 : 04       POP                     ; ; @ 1\n"
        "    7 : 19 00 01 JUMP +1 -> 11           ; if @ 1\n"
        "   10 : 04       POP                     ; if @ 1\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(if_else_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "if (true) nil; else nil;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; true @ 1\n"
        "    1 : 1a 00 06 JUMP_IF_FALSE +6 -> 10  ; if @ 1\n"
        "    4 : 04       POP                     ; if @ 1\n"
        "    5 : 01       NIL                     ; nil @ 1\n"
        "    6 : 04       POP                     ; ; @ 1\n"
        "    7 : 19 00 03 JUMP +3 -> 13           ; if @ 1\n"
        "   10 : 04       POP                     ; if @ 1\n"
        "   11 : 01       NIL                     ; nil @ 1\n"
        "   12 : 04       POP                     ; ; @ 1\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(if_block_will_create_a_scope)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "{ var x = 42; if (true) { var x = 14; } x; }");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 02       TRUE                    ; true @ 1\n"
        "    3 : 1a 00 07 JUMP_IF_FALSE +7 -> 13  ; if @ 1\n"
        "    6 : 04       POP                     ; if @ 1\n"
        "    7 : 00 01    CONSTANT [1]            ; 14 @ 1\n"
        "    9 : 04       POP                     ; } @ 1\n"
        "   10 : 19 00 01 JUMP +1 -> 14           ; if @ 1\n"
        "   13 : 04       POP                     ; if @ 1\n"
        "   14 : 05 00    GET_LOCAL [0]           ; x @ 1\n"
        "   16 : 04       POP                     ; ; @ 1\n"
        "   17 : 04       POP                     ; } @ 1\n"
        "Constants:\n"
        "    0 : 42\n"
        "    1 : 14\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(for_loops_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "for (var x = 0; x != 3; x = x + 1) nil;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 0 @ 1\n"
        "    2 : 05 00    GET_LOCAL [0]           ; x @ 1\n"
        "    4 : 00 01    CONSTANT [1]            ; 3 @ 1\n"
        "    6 : 0f       EQUAL                   ; != @ 1\n"
        "    7 : 16       NOT                     ; != @ 1\n"
        "    8 : 1a 00 14 JUMP_IF_FALSE +20 -> 31 ; for @ 1\n"
        "   11 : 19 00 0b JUMP +11 -> 25          ; for @ 1\n"
        "   14 : 05 00    GET_LOCAL [0]           ; x @ 1\n"
        "   16 : 00 02    CONSTANT [2]            ; 1 @ 1\n"
        "   18 : 12       ADD                     ; + @ 1\n"
        "   19 : 06 00    SET_LOCAL [0]           ; x @ 1\n"
        "   21 : 04       POP                     ; for @ 1\n"
        "   22 : 1b 00 17 LOOP -23 -> 2           ; for @ 1\n"
        "   25 : 04       POP                     ; for @ 1\n"
        "   26 : 01       NIL                     ; nil @ 1\n"
        "   27 : 04       POP                     ; ; @ 1\n"
        "   28 : 1b 00 11 LOOP -17 -> 14          ; for @ 1\n"
        "   31 : 04       POP                     ; for @ 1\n"
        "   32 : 04       POP                     ; for @ 1\n"
        "Constants:\n"
        "    0 : 0\n"
        "    1 : 3\n"
        "    2 : 1\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(for_loop_init_condition_increment_can_be_blank)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "for (;;) nil;");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; for @ 1\n"
        "    1 : 1a 00 0c JUMP_IF_FALSE +12 -> 16 ; for @ 1\n"
        "    4 : 19 00 03 JUMP +3 -> 10           ; for @ 1\n"
        "    7 : 1b 00 0a LOOP -10 -> 0           ; for @ 1\n"
        "   10 : 04       POP                     ; for @ 1\n"
        "   11 : 01       NIL                     ; nil @ 1\n"
        "   12 : 04       POP                     ; ; @ 1\n"
        "   13 : 1b 00 09 LOOP -9 -> 7            ; for @ 1\n"
        "   16 : 04       POP                     ; for @ 1\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(for_loop_vars_will_be_local)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "{ var x = 42; for (var x = 0; x != 3; x = x + 1) nil; }");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 0 @ 1\n"
        "    4 : 05 01    GET_LOCAL [1]           ; x @ 1\n"
        "    6 : 00 02    CONSTANT [2]            ; 3 @ 1\n"
        "    8 : 0f       EQUAL                   ; != @ 1\n"
        "    9 : 16       NOT                     ; != @ 1\n"
        "   10 : 1a 00 14 JUMP_IF_FALSE +20 -> 33 ; for @ 1\n"
        "   13 : 19 00 0b JUMP +11 -> 27          ; for @ 1\n"
        "   16 : 05 01    GET_LOCAL [1]           ; x @ 1\n"
        "   18 : 00 03    CONSTANT [3]            ; 1 @ 1\n"
        "   20 : 12       ADD                     ; + @ 1\n"
        "   21 : 06 01    SET_LOCAL [1]           ; x @ 1\n"
        "   23 : 04       POP                     ; for @ 1\n"
        "   24 : 1b 00 17 LOOP -23 -> 4           ; for @ 1\n"
        "   27 : 04       POP                     ; for @ 1\n"
        "   28 : 01       NIL                     ; nil @ 1\n"
        "   29 : 04       POP                     ; ; @ 1\n"
        "   30 : 1b 00 11 LOOP -17 -> 16          ; for @ 1\n"
        "   33 : 04       POP                     ; for @ 1\n"
        "   34 : 04       POP                     ; for @ 1\n"
        "   35 : 04       POP                     ; } @ 1\n"
        "Constants:\n"
        "    0 : 42\n"
        "    1 : 0\n"
        "    2 : 3\n"
        "    3 : 1\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(non_var_statements_in_for_loop_init_will_throw)
{
    const auto expect_to_throw = [] {
        motts::lox::GC_heap gc_heap;
        motts::lox::Interned_strings interned_strings{gc_heap};
        compile(gc_heap, interned_strings, "for (print x;;) nil;");
    };

    BOOST_CHECK_THROW(expect_to_throw(), std::runtime_error);

    try {
        expect_to_throw();
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error: Unexpected token \"print\".");
    }
}

BOOST_AUTO_TEST_CASE(function_declaration_and_invocation_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(
        gc_heap,
        interned_strings,
        "fun f() {}\n"
        "f();\n"
        "{ fun g() {} }\n"
    );
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 1f 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 08 01    DEFINE_GLOBAL [1]       ; fun @ 1\n"
        "    5 : 07 01    GET_GLOBAL [1]          ; f @ 2\n"
        "    7 : 1c 00    CALL (0)                ; f @ 2\n"
        "    9 : 04       POP                     ; ; @ 2\n"
        "   10 : 1f 02 00 CLOSURE [2] (0)         ; fun @ 3\n"
        "   13 : 04       POP                     ; } @ 3\n"
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : f\n"
        "    2 : <fn g>\n"
        "[<fn f> chunk]\n"
        "Bytecode:\n"
        "    0 : 01       NIL                     ; fun @ 1\n"
        "    1 : 21       RETURN                  ; fun @ 1\n"
        "Constants:\n"
        "    -\n"
        "[<fn g> chunk]\n"
        "Bytecode:\n"
        "    0 : 01       NIL                     ; fun @ 3\n"
        "    1 : 21       RETURN                  ; fun @ 3\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(function_expr_and_invocation_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(
        gc_heap,
        interned_strings,
        "var f = fun () {};\n"
        "f();\n"
    );
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 1f 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 08 01    DEFINE_GLOBAL [1]       ; var @ 1\n"
        "    5 : 07 01    GET_GLOBAL [1]          ; f @ 2\n"
        "    7 : 1c 00    CALL (0)                ; f @ 2\n"
        "    9 : 04       POP                     ; ; @ 2\n"
        "Constants:\n"
        "    0 : <fn (anonymous)>\n"
        "    1 : f\n"
        "[<fn (anonymous)> chunk]\n"
        "Bytecode:\n"
        "    0 : 01       NIL                     ; fun @ 1\n"
        "    1 : 21       RETURN                  ; fun @ 1\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(function_parameters_arguments_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(
        gc_heap,
        interned_strings,
        "fun f(x) { x; }\n"
        "f(42);\n"
    );
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 1f 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 08 01    DEFINE_GLOBAL [1]       ; fun @ 1\n"
        "    5 : 07 01    GET_GLOBAL [1]          ; f @ 2\n"
        "    7 : 00 02    CONSTANT [2]            ; 42 @ 2\n"
        "    9 : 1c 01    CALL (1)                ; f @ 2\n"
        "   11 : 04       POP                     ; ; @ 2\n"
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : f\n"
        "    2 : 42\n"
        "[<fn f> chunk]\n"
        "Bytecode:\n"
        "    0 : 05 01    GET_LOCAL [1]           ; x @ 1\n"
        "    2 : 04       POP                     ; ; @ 1\n"
        "    3 : 01       NIL                     ; fun @ 1\n"
        "    4 : 21       RETURN                  ; fun @ 1\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(function_return_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(
        gc_heap,
        interned_strings,
        "fun f(x) { return x; }\n"
        "f(42);\n"
    );
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 1f 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 08 01    DEFINE_GLOBAL [1]       ; fun @ 1\n"
        "    5 : 07 01    GET_GLOBAL [1]          ; f @ 2\n"
        "    7 : 00 02    CONSTANT [2]            ; 42 @ 2\n"
        "    9 : 1c 01    CALL (1)                ; f @ 2\n"
        "   11 : 04       POP                     ; ; @ 2\n"
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : f\n"
        "    2 : 42\n"
        "[<fn f> chunk]\n"
        "Bytecode:\n"
        "    0 : 05 01    GET_LOCAL [1]           ; x @ 1\n"
        "    2 : 21       RETURN                  ; return @ 1\n"
        "    3 : 01       NIL                     ; fun @ 1\n"
        "    4 : 21       RETURN                  ; fun @ 1\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(empty_return_will_return_nil)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(
        gc_heap,
        interned_strings,
        "fun f() { return; }\n"
        "f();\n"
    );
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 1f 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 08 01    DEFINE_GLOBAL [1]       ; fun @ 1\n"
        "    5 : 07 01    GET_GLOBAL [1]          ; f @ 2\n"
        "    7 : 1c 00    CALL (0)                ; f @ 2\n"
        "    9 : 04       POP                     ; ; @ 2\n"
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : f\n"
        "[<fn f> chunk]\n"
        "Bytecode:\n"
        "    0 : 01       NIL                     ; return @ 1\n"
        "    1 : 21       RETURN                  ; return @ 1\n"
        "    2 : 01       NIL                     ; fun @ 1\n"
        "    3 : 21       RETURN                  ; fun @ 1\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(return_outside_function_will_throw)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};

    BOOST_CHECK_THROW(compile(gc_heap, interned_strings, "return;"), std::runtime_error);
    try {
        compile(gc_heap, interned_strings, "return;");
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"return\": Can't return from top-level code.");
    }
}

BOOST_AUTO_TEST_CASE(function_body_will_have_local_access_to_original_function_name)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(gc_heap, interned_strings, "fun f() { f; }\n");
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 1f 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 08 01    DEFINE_GLOBAL [1]       ; fun @ 1\n"
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : f\n"
        "[<fn f> chunk]\n"
        "Bytecode:\n"
        "    0 : 05 00    GET_LOCAL [0]           ; f @ 1\n"
        "    2 : 04       POP                     ; ; @ 1\n"
        "    3 : 01       NIL                     ; fun @ 1\n"
        "    4 : 21       RETURN                  ; fun @ 1\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(inner_functions_can_capture_enclosing_local_variables)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(
        gc_heap,
        interned_strings,
        "fun outer() {\n"
        "    var x = 42;\n"
        "    fun middle() {\n"
        "        fun inner() {\n"
        "            x = 108;\n"
        "            print x;\n"
        "        }\n"
        "        inner();\n"
        "    }\n"
        "    middle();\n"
        "}\n"
    );
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 1f 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 08 01    DEFINE_GLOBAL [1]       ; fun @ 1\n"
        "Constants:\n"
        "    0 : <fn outer>\n"
        "    1 : outer\n"
        // fun outer() {
        "[<fn outer> chunk]\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 2\n"
        "    2 : 1f 01 01 CLOSURE [1] (1)         ; fun @ 3\n"
        "           01 01 | ^ [1]                 ; fun @ 3\n"
        "    7 : 05 02    GET_LOCAL [2]           ; middle @ 10\n"
        "    9 : 1c 00    CALL (0)                ; middle @ 10\n"
        "   11 : 04       POP                     ; ; @ 10\n"
        "   12 : 01       NIL                     ; fun @ 1\n"
        "   13 : 21       RETURN                  ; fun @ 1\n"
        "Constants:\n"
        "    0 : 42\n"
        "    1 : <fn middle>\n"
        // fun middle() {
        "[<fn middle> chunk]\n"
        "Bytecode:\n"
        "    0 : 1f 00 01 CLOSURE [0] (1)         ; fun @ 4\n"
        "           00 00 | ^^ [0]                ; fun @ 4\n"
        "    5 : 05 01    GET_LOCAL [1]           ; inner @ 8\n"
        "    7 : 1c 00    CALL (0)                ; inner @ 8\n"
        "    9 : 04       POP                     ; ; @ 8\n"
        "   10 : 01       NIL                     ; fun @ 3\n"
        "   11 : 21       RETURN                  ; fun @ 3\n"
        "Constants:\n"
        "    0 : <fn inner>\n"
        // fun inner() {
        "[<fn inner> chunk]\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 108 @ 5\n"
        "    2 : 0b 00    SET_UPVALUE [0]         ; x @ 5\n"
        "    4 : 04       POP                     ; ; @ 5\n"
        "    5 : 0a 00    GET_UPVALUE [0]         ; x @ 6\n"
        "    7 : 18       PRINT                   ; print @ 6\n"
        "    8 : 01       NIL                     ; fun @ 4\n"
        "    9 : 21       RETURN                  ; fun @ 4\n"
        "Constants:\n"
        "    0 : 108\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(closed_variables_stay_alive_even_after_function_has_returned)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(
        gc_heap,
        interned_strings,
        "fun outer() {\n"
        "    var x = 42;\n"
        "    fun middle() {\n"
        "        fun inner() {\n"
        "            x = 108;\n"
        "            print x;\n"
        "        }\n"
        "        inner();\n"
        "    }\n"
        "    return middle;\n"
        "}\n"
        "var closure = outer();\n"
        "closure();"
    );
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        "    0 : 1f 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 08 01    DEFINE_GLOBAL [1]       ; fun @ 1\n"
        "    5 : 07 01    GET_GLOBAL [1]          ; outer @ 12\n"
        "    7 : 1c 00    CALL (0)                ; outer @ 12\n"
        "    9 : 08 02    DEFINE_GLOBAL [2]       ; var @ 12\n"
        "   11 : 07 02    GET_GLOBAL [2]          ; closure @ 13\n"
        "   13 : 1c 00    CALL (0)                ; closure @ 13\n"
        "   15 : 04       POP                     ; ; @ 13\n"
        "Constants:\n"
        "    0 : <fn outer>\n"
        "    1 : outer\n"
        "    2 : closure\n"
        // fun outer() {
        "[<fn outer> chunk]\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 2\n"
        "    2 : 1f 01 01 CLOSURE [1] (1)         ; fun @ 3\n"
        "           01 01 | ^ [1]                 ; fun @ 3\n"
        "    7 : 05 02    GET_LOCAL [2]           ; middle @ 10\n"
        "    9 : 21       RETURN                  ; return @ 10\n"
        "   10 : 01       NIL                     ; fun @ 1\n"
        "   11 : 21       RETURN                  ; fun @ 1\n"
        "Constants:\n"
        "    0 : 42\n"
        "    1 : <fn middle>\n"
        // fun middle() {
        "[<fn middle> chunk]\n"
        "Bytecode:\n"
        "    0 : 1f 00 01 CLOSURE [0] (1)         ; fun @ 4\n"
        "           00 00 | ^^ [0]                ; fun @ 4\n"
        "    5 : 05 01    GET_LOCAL [1]           ; inner @ 8\n"
        "    7 : 1c 00    CALL (0)                ; inner @ 8\n"
        "    9 : 04       POP                     ; ; @ 8\n"
        "   10 : 01       NIL                     ; fun @ 3\n"
        "   11 : 21       RETURN                  ; fun @ 3\n"
        "Constants:\n"
        "    0 : <fn inner>\n"
        // fun inner() {
        "[<fn inner> chunk]\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 108 @ 5\n"
        "    2 : 0b 00    SET_UPVALUE [0]         ; x @ 5\n"
        "    4 : 04       POP                     ; ; @ 5\n"
        "    5 : 0a 00    GET_UPVALUE [0]         ; x @ 6\n"
        "    7 : 18       PRINT                   ; print @ 6\n"
        "    8 : 01       NIL                     ; fun @ 4\n"
        "    9 : 21       RETURN                  ; fun @ 4\n"
        "Constants:\n"
        "    0 : 108\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(class_will_compile)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(
        gc_heap,
        interned_strings,
        "class Klass {\n"
        "    method() {}\n"
        "}\n"
        "var instance = Klass();\n"
        "instance.property = 42;\n"
        "instance.property;\n"
    );
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        // class Klass
        "    0 : 22 00    CLASS [0]               ; class @ 1\n"
        "    2 : 1f 01 00 CLOSURE [1] (0)         ; method @ 2\n"
        "    5 : 24 02    METHOD [2]              ; method @ 2\n"
        "    7 : 08 00    DEFINE_GLOBAL [0]       ; class @ 1\n"
        // var instance = Klass();
        "    9 : 07 00    GET_GLOBAL [0]          ; Klass @ 4\n"
        "   11 : 1c 00    CALL (0)                ; Klass @ 4\n"
        "   13 : 08 03    DEFINE_GLOBAL [3]       ; var @ 4\n"
        // instance.property = 42;
        "   15 : 00 04    CONSTANT [4]            ; 42 @ 5\n"
        "   17 : 07 03    GET_GLOBAL [3]          ; instance @ 5\n"
        "   19 : 0d 05    SET_PROPERTY [5]        ; property @ 5\n"
        "   21 : 04       POP                     ; ; @ 5\n"
        // instance.property;
        "   22 : 07 03    GET_GLOBAL [3]          ; instance @ 6\n"
        "   24 : 0c 05    GET_PROPERTY [5]        ; property @ 6\n"
        "   26 : 04       POP                     ; ; @ 6\n"
        "Constants:\n"
        "    0 : Klass\n"
        "    1 : <fn method>\n"
        "    2 : method\n"
        "    3 : instance\n"
        "    4 : 42\n"
        "    5 : property\n"
        "[<fn method> chunk]\n"
        "Bytecode:\n"
        "    0 : 01       NIL                     ; method @ 2\n"
        "    1 : 21       RETURN                  ; method @ 2\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(class_name_can_be_used_in_methods)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(
        gc_heap,
        interned_strings,
        "class GlobalClass {\n"
        "    method() {\n"
        "        GlobalClass;\n"
        "    }\n"
        "}\n"
        "{\n"
        "    class LocalClass {\n"
        "        method() {\n"
        "            LocalClass;\n"
        "        }\n"
        "    }\n"
        "}\n"
    );
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        // class GlobalClass {
        //     method()
        // }
        "    0 : 22 00    CLASS [0]               ; class @ 1\n"
        "    2 : 1f 01 00 CLOSURE [1] (0)         ; method @ 2\n"
        "    5 : 24 02    METHOD [2]              ; method @ 2\n"
        "    7 : 08 00    DEFINE_GLOBAL [0]       ; class @ 1\n"
        // {
        //     class LocalClass {
        //         method()
        //     }
        // }
        "    9 : 22 03    CLASS [3]               ; class @ 7\n"
        "   11 : 1f 04 01 CLOSURE [4] (1)         ; method @ 8\n"
        "           01 00 | ^ [0]                 ; method @ 8\n"
        "   16 : 24 02    METHOD [2]              ; method @ 8\n"
        "   18 : 20       CLOSE_UPVALUE           ; } @ 12\n"
        "Constants:\n"
        "    0 : GlobalClass\n"
        "    1 : <fn method>\n"
        "    2 : method\n"
        "    3 : LocalClass\n"
        "    4 : <fn method>\n"
        "[<fn method> chunk]\n"
        "Bytecode:\n"
        "    0 : 07 00    GET_GLOBAL [0]          ; GlobalClass @ 3\n"
        "    2 : 04       POP                     ; ; @ 3\n"
        "    3 : 01       NIL                     ; method @ 2\n"
        "    4 : 21       RETURN                  ; method @ 2\n"
        "Constants:\n"
        "    0 : GlobalClass\n"
        "[<fn method> chunk]\n"
        "Bytecode:\n"
        "    0 : 0a 00    GET_UPVALUE [0]         ; LocalClass @ 9\n"
        "    2 : 04       POP                     ; ; @ 9\n"
        "    3 : 01       NIL                     ; method @ 8\n"
        "    4 : 21       RETURN                  ; method @ 8\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(property_access_can_be_chained)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(
        gc_heap,
        interned_strings,
        "class Klass {}\n"
        "var globalFoo = Klass();\n"
        "globalFoo.bar = Klass();\n"
        "globalFoo.bar.baz = 42;\n"
        "print globalFoo.bar.baz;\n"
        "{\n"
        "    var localFoo = Klass();\n"
        "    localFoo.bar = Klass();\n"
        "    localFoo.bar.baz = 42;\n"
        "    print localFoo.bar.baz;\n"
        "    fun f() {\n"
        "        localFoo.bar.baz = 108;\n"
        "        print localFoo.bar.baz;\n"
        "    }\n"
        "    f();\n"
        "}\n"
    );
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        // class Klass {}
        "    0 : 22 00    CLASS [0]               ; class @ 1\n"
        "    2 : 08 00    DEFINE_GLOBAL [0]       ; class @ 1\n"
        // var globalFoo = Klass();
        "    4 : 07 00    GET_GLOBAL [0]          ; Klass @ 2\n"
        "    6 : 1c 00    CALL (0)                ; Klass @ 2\n"
        "    8 : 08 01    DEFINE_GLOBAL [1]       ; var @ 2\n"
        // globalFoo.bar = Klass();
        "   10 : 07 00    GET_GLOBAL [0]          ; Klass @ 3\n"
        "   12 : 1c 00    CALL (0)                ; Klass @ 3\n"
        "   14 : 07 01    GET_GLOBAL [1]          ; globalFoo @ 3\n"
        "   16 : 0d 02    SET_PROPERTY [2]        ; bar @ 3\n"
        "   18 : 04       POP                     ; ; @ 3\n"
        // globalFoo.bar.baz = 42;
        "   19 : 00 03    CONSTANT [3]            ; 42 @ 4\n"
        "   21 : 07 01    GET_GLOBAL [1]          ; globalFoo @ 4\n"
        "   23 : 0c 02    GET_PROPERTY [2]        ; bar @ 4\n"
        "   25 : 0d 04    SET_PROPERTY [4]        ; baz @ 4\n"
        "   27 : 04       POP                     ; ; @ 4\n"
        // print globalFoo.bar.baz;
        "   28 : 07 01    GET_GLOBAL [1]          ; globalFoo @ 5\n"
        "   30 : 0c 02    GET_PROPERTY [2]        ; bar @ 5\n"
        "   32 : 0c 04    GET_PROPERTY [4]        ; baz @ 5\n"
        "   34 : 18       PRINT                   ; print @ 5\n"
        // var localFoo = Klass();
        "   35 : 07 00    GET_GLOBAL [0]          ; Klass @ 7\n"
        "   37 : 1c 00    CALL (0)                ; Klass @ 7\n"
        // localFoo.bar = Klass();
        "   39 : 07 00    GET_GLOBAL [0]          ; Klass @ 8\n"
        "   41 : 1c 00    CALL (0)                ; Klass @ 8\n"
        "   43 : 05 00    GET_LOCAL [0]           ; localFoo @ 8\n"
        "   45 : 0d 02    SET_PROPERTY [2]        ; bar @ 8\n"
        "   47 : 04       POP                     ; ; @ 8\n"
        // localFoo.bar.baz = 42;
        "   48 : 00 03    CONSTANT [3]            ; 42 @ 9\n"
        "   50 : 05 00    GET_LOCAL [0]           ; localFoo @ 9\n"
        "   52 : 0c 02    GET_PROPERTY [2]        ; bar @ 9\n"
        "   54 : 0d 04    SET_PROPERTY [4]        ; baz @ 9\n"
        "   56 : 04       POP                     ; ; @ 9\n"
        // print localFoo.bar.baz;
        "   57 : 05 00    GET_LOCAL [0]           ; localFoo @ 10\n"
        "   59 : 0c 02    GET_PROPERTY [2]        ; bar @ 10\n"
        "   61 : 0c 04    GET_PROPERTY [4]        ; baz @ 10\n"
        "   63 : 18       PRINT                   ; print @ 10\n"
        // fun f()
        "   64 : 1f 05 01 CLOSURE [5] (1)         ; fun @ 11\n"
        "           01 00 | ^ [0]                 ; fun @ 11\n"
        // f();
        "   69 : 05 01    GET_LOCAL [1]           ; f @ 15\n"
        "   71 : 1c 00    CALL (0)                ; f @ 15\n"
        "   73 : 04       POP                     ; ; @ 15\n"
        // }
        "   74 : 04       POP                     ; } @ 16\n"
        "   75 : 20       CLOSE_UPVALUE           ; } @ 16\n"
        // fun f()
        "Constants:\n"
        "    0 : Klass\n"
        "    1 : globalFoo\n"
        "    2 : bar\n"
        "    3 : 42\n"
        "    4 : baz\n"
        "    5 : <fn f>\n"
        "[<fn f> chunk]\n"
        "Bytecode:\n"
        // localFoo.bar.baz = 108;
        "    0 : 00 00    CONSTANT [0]            ; 108 @ 12\n"
        "    2 : 0a 00    GET_UPVALUE [0]         ; localFoo @ 12\n"
        "    4 : 0c 01    GET_PROPERTY [1]        ; bar @ 12\n"
        "    6 : 0d 02    SET_PROPERTY [2]        ; baz @ 12\n"
        "    8 : 04       POP                     ; ; @ 12\n"
        // print localFoo.bar.baz;
        "    9 : 0a 00    GET_UPVALUE [0]         ; localFoo @ 13\n"
        "   11 : 0c 01    GET_PROPERTY [1]        ; bar @ 13\n"
        "   13 : 0c 02    GET_PROPERTY [2]        ; baz @ 13\n"
        "   15 : 18       PRINT                   ; print @ 13\n"
        // }
        "   16 : 01       NIL                     ; fun @ 11\n"
        "   17 : 21       RETURN                  ; fun @ 11\n"
        "Constants:\n"
        "    0 : 108\n"
        "    1 : bar\n"
        "    2 : baz\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(class_methods_can_access_and_capture_this)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(
        gc_heap,
        interned_strings,
        "class Klass {\n"
        "    method() {\n"
        "        fun inner() {\n"
        "            print this;\n"
        "        }\n"
        "        return inner;\n"
        "    }\n"
        "}\n"
        "Klass().method()();\n"
    );
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        // class Klass {
        //     method()
        // }
        "    0 : 22 00    CLASS [0]               ; class @ 1\n"
        "    2 : 1f 01 00 CLOSURE [1] (0)         ; method @ 2\n"
        "    5 : 24 02    METHOD [2]              ; method @ 2\n"
        "    7 : 08 00    DEFINE_GLOBAL [0]       ; class @ 1\n"
        // Klass().method()();
        "    9 : 07 00    GET_GLOBAL [0]          ; Klass @ 9\n"
        "   11 : 1c 00    CALL (0)                ; Klass @ 9\n"
        "   13 : 0c 02    GET_PROPERTY [2]        ; method @ 9\n"
        "   15 : 1c 00    CALL (0)                ; method @ 9\n"
        "   17 : 1c 00    CALL (0)                ; method @ 9\n"
        "   19 : 04       POP                     ; ; @ 9\n"
        "Constants:\n"
        "    0 : Klass\n"
        "    1 : <fn method>\n"
        "    2 : method\n"
        // method() {
        "[<fn method> chunk]\n"
        "Bytecode:\n"
        // fun inner()
        "    0 : 1f 00 01 CLOSURE [0] (1)         ; fun @ 3\n"
        "           01 00 | ^ [0]                 ; fun @ 3\n"
        // return inner;
        "    5 : 05 01    GET_LOCAL [1]           ; inner @ 6\n"
        "    7 : 21       RETURN                  ; return @ 6\n"
        // }
        "    8 : 01       NIL                     ; method @ 2\n"
        "    9 : 21       RETURN                  ; method @ 2\n"
        "Constants:\n"
        "    0 : <fn inner>\n"
        // fun inner() {
        "[<fn inner> chunk]\n"
        "Bytecode:\n"
        // print this;
        "    0 : 0a 00    GET_UPVALUE [0]         ; this @ 4\n"
        "    2 : 18       PRINT                   ; print @ 4\n"
        // }
        "    3 : 01       NIL                     ; fun @ 3\n"
        "    4 : 21       RETURN                  ; fun @ 3\n"
        "Constants:\n"
        "    -\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(class_init_method_will_implicitly_return_instance)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(
        gc_heap,
        interned_strings,
        "class Klass {\n"
        "    init() {\n"
        "        this.property = 42;\n"
        "        return;\n"
        "    }\n"
        "}\n"
    );
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        // class Klass {
        //     init()
        // }
        "    0 : 22 00    CLASS [0]               ; class @ 1\n"
        "    2 : 1f 01 00 CLOSURE [1] (0)         ; init @ 2\n"
        "    5 : 24 02    METHOD [2]              ; init @ 2\n"
        "    7 : 08 00    DEFINE_GLOBAL [0]       ; class @ 1\n"
        "Constants:\n"
        "    0 : Klass\n"
        "    1 : <fn init>\n"
        "    2 : init\n"
        // init() {
        "[<fn init> chunk]\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 3\n"
        "    2 : 05 00    GET_LOCAL [0]           ; this @ 3\n"
        "    4 : 0d 01    SET_PROPERTY [1]        ; property @ 3\n"
        "    6 : 04       POP                     ; ; @ 3\n"
        "    7 : 05 00    GET_LOCAL [0]           ; this @ 4\n"
        "    9 : 21       RETURN                  ; return @ 4\n"
        "   10 : 05 00    GET_LOCAL [0]           ; this @ 2\n"
        "   12 : 21       RETURN                  ; init @ 2\n"
        "Constants:\n"
        "    0 : 42\n"
        "    1 : property\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(returning_value_in_class_init_will_throw)
{
    const auto expect_to_throw = [] {
        motts::lox::GC_heap gc_heap;
        motts::lox::Interned_strings interned_strings{gc_heap};
        compile(
            gc_heap,
            interned_strings,
            "class Klass {\n"
            "    init() {\n"
            "        return 42;\n"
            "    }\n"
            "}\n"
        );
    };

    BOOST_CHECK_THROW(expect_to_throw(), std::runtime_error);
    try {
        expect_to_throw();
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 3] Error at \"return\": Can't return a value from an initializer.");
    }
}

BOOST_AUTO_TEST_CASE(classes_can_inherit_from_classes)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(
        gc_heap,
        interned_strings,
        "class Parent {}\n"
        "class Child < Parent {}\n"
    );
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        // class Parent
        "    0 : 22 00    CLASS [0]               ; class @ 1\n"
        "    2 : 08 00    DEFINE_GLOBAL [0]       ; class @ 1\n"
        // class Child < Parent
        "    4 : 22 01    CLASS [1]               ; class @ 2\n"
        "    6 : 07 00    GET_GLOBAL [0]          ; Parent @ 2\n"
        "    8 : 23       INHERIT                 ; Parent @ 2\n"
        "    9 : 04       POP                     ; Parent @ 2\n"
        "   10 : 04       POP                     ; super @ 2\n"
        "   11 : 08 01    DEFINE_GLOBAL [1]       ; class @ 2\n"
        "Constants:\n"
        "    0 : Parent\n"
        "    1 : Child\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(class_inheriting_from_itself_will_throw)
{
    const auto expect_to_throw = [] {
        motts::lox::GC_heap gc_heap;
        motts::lox::Interned_strings interned_strings{gc_heap};
        compile(gc_heap, interned_strings, "class Klass < Klass {}\n");
    };

    BOOST_CHECK_THROW(expect_to_throw(), std::runtime_error);
    try {
        expect_to_throw();
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error: A class can't inherit from itself.");
    }
}

BOOST_AUTO_TEST_CASE(subclass_can_access_super)
{
    motts::lox::GC_heap gc_heap;
    motts::lox::Interned_strings interned_strings{gc_heap};
    const auto root_fn = compile(
        gc_heap,
        interned_strings,
        "class Parent {\n"
        "    method() {}\n"
        "}\n"
        "class Child < Parent {\n"
        "    method() {\n"
        "        super.method();\n"
        "    }\n"
        "}\n"
    );
    const auto& chunk = root_fn->chunk;

    std::ostringstream os;
    os << chunk;

    // clang-format off
    const auto* expected =
        "Bytecode:\n"
        // class Parent {
        //     method()
        // }
        "    0 : 22 00    CLASS [0]               ; class @ 1\n"
        "    2 : 1f 01 00 CLOSURE [1] (0)         ; method @ 2\n"
        "    5 : 24 02    METHOD [2]              ; method @ 2\n"
        "    7 : 08 00    DEFINE_GLOBAL [0]       ; class @ 1\n"
        // class Child < Parent {
        //     method()
        // }
        "    9 : 22 03    CLASS [3]               ; class @ 4\n"
        "   11 : 07 00    GET_GLOBAL [0]          ; Parent @ 4\n"
        "   13 : 23       INHERIT                 ; Parent @ 4\n"
        "   14 : 1f 04 01 CLOSURE [4] (1)         ; method @ 5\n"
        "           01 01 | ^ [1]                 ; method @ 5\n"
        "   19 : 24 02    METHOD [2]              ; method @ 5\n"
        "   21 : 04       POP                     ; Parent @ 4\n"
        "   22 : 20       CLOSE_UPVALUE           ; super @ 4\n"
        "   23 : 08 03    DEFINE_GLOBAL [3]       ; class @ 4\n"
        "Constants:\n"
        "    0 : Parent\n"
        "    1 : <fn method>\n"
        "    2 : method\n"
        "    3 : Child\n"
        "    4 : <fn method>\n"
        // Parent::method() {
        "[<fn method> chunk]\n"
        "Bytecode:\n"
        "    0 : 01       NIL                     ; method @ 2\n"
        "    1 : 21       RETURN                  ; method @ 2\n"
        "Constants:\n"
        "    -\n"
        // Child::method() {
        "[<fn method> chunk]\n"
        "Bytecode:\n"
        "    0 : 05 00    GET_LOCAL [0]           ; this @ 6\n"
        "    2 : 0a 00    GET_UPVALUE [0]         ; super @ 6\n"
        "    4 : 0e 00    GET_SUPER [0]           ; method @ 6\n"
        "    6 : 1c 00    CALL (0)                ; super @ 6\n"
        "    8 : 04       POP                     ; ; @ 6\n"
        "    9 : 01       NIL                     ; method @ 5\n"
        "   10 : 21       RETURN                  ; method @ 5\n"
        "Constants:\n"
        "    0 : method\n";
    // clang-format on

    BOOST_TEST(os.str() == expected);
}
