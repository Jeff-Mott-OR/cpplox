#define BOOST_TEST_MODULE Compiler Tests
#include <boost/test/unit_test.hpp>

#include <sstream>
#include <stdexcept>

#include "../src/compiler.hpp"

using motts::lox::Dynamic_type_value;
using motts::lox::Opcode;
using motts::lox::Token;
using motts::lox::Token_type;

BOOST_AUTO_TEST_CASE(opcodes_can_be_printed) {
    std::ostringstream os;
    os << Opcode::constant;
    BOOST_TEST(os.str() == "CONSTANT");
}

BOOST_AUTO_TEST_CASE(printing_invalid_opcode_will_throw) {
    union Invalid_opcode {
        Opcode as_opcode;
        int as_int;
    };
    Invalid_opcode invalid_opcode;
    invalid_opcode.as_int = 276'709; // Don't panic.

    std::ostringstream os;
    BOOST_CHECK_THROW(os << invalid_opcode.as_opcode, std::exception);
}

BOOST_AUTO_TEST_CASE(chunks_can_be_printed) {
    motts::lox::Chunk chunk;
    chunk.emit_constant(Dynamic_type_value{42.0}, Token{Token_type::number, "42", 1});
    chunk.emit<Opcode::nil>(Token{Token_type::nil, "nil", 2});

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 42\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 01       NIL                     ; nil @ 2\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(number_literals_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "42;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 42\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(nil_literals_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "nil;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 01       NIL                     ; nil @ 1\n"
        "    1 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(invalid_expressions_will_throw) {
    motts::lox::GC_heap gc_heap;
    BOOST_CHECK_THROW(compile(gc_heap, "?"), std::exception);

    try {
        compile(gc_heap, "?");
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error: Unexpected character \"?\".");
    }
}

BOOST_AUTO_TEST_CASE(true_false_literals_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "true; false;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; true @ 1\n"
        "    1 : 0b       POP                     ; ; @ 1\n"
        "    2 : 03       FALSE                   ; false @ 1\n"
        "    3 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(string_literals_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "\"hello\";");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : hello\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; \"hello\" @ 1\n"
        "    2 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(addition_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "28 + 14;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 28\n"
        "    1 : 14\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 28 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 14 @ 1\n"
        "    4 : 04       ADD                     ; + @ 1\n"
        "    5 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(invalid_addition_will_throw) {
    motts::lox::GC_heap gc_heap;
    BOOST_CHECK_THROW(compile(gc_heap, "42 + "), std::exception);

    try {
        compile(gc_heap, "42 + ");
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error: Unexpected token \"EOF\".");
    }
}

BOOST_AUTO_TEST_CASE(print_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "print 42;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 42\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 05       PRINT                   ; print @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(plus_minus_star_slash_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "1 + 2 - 3 * 5 / 7;");

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
        "    0 : 00 00    CONSTANT [0]            ; 1 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 2 @ 1\n"
        "    4 : 04       ADD                     ; + @ 1\n"
        "    5 : 00 02    CONSTANT [2]            ; 3 @ 1\n"
        "    7 : 00 03    CONSTANT [3]            ; 5 @ 1\n"
        "    9 : 07       MULTIPLY                ; * @ 1\n"
        "   10 : 00 04    CONSTANT [4]            ; 7 @ 1\n"
        "   12 : 08       DIVIDE                  ; / @ 1\n"
        "   13 : 06       SUBTRACT                ; - @ 1\n"
        "   14 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(parens_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "1 + (2 - 3) * 5 / 7;");

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
        "    0 : 00 00    CONSTANT [0]            ; 1 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 2 @ 1\n"
        "    4 : 00 02    CONSTANT [2]            ; 3 @ 1\n"
        "    6 : 06       SUBTRACT                ; - @ 1\n"
        "    7 : 00 03    CONSTANT [3]            ; 5 @ 1\n"
        "    9 : 07       MULTIPLY                ; * @ 1\n"
        "   10 : 00 04    CONSTANT [4]            ; 7 @ 1\n"
        "   12 : 08       DIVIDE                  ; / @ 1\n"
        "   13 : 04       ADD                     ; + @ 1\n"
        "   14 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(numeric_negation_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "-1 + -1;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 1\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 1 @ 1\n"
        "    2 : 09       NEGATE                  ; - @ 1\n"
        "    3 : 00 00    CONSTANT [0]            ; 1 @ 1\n"
        "    5 : 09       NEGATE                  ; - @ 1\n"
        "    6 : 04       ADD                     ; + @ 1\n"
        "    7 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(boolean_negation_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "!true;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; true @ 1\n"
        "    1 : 0a       NOT                     ; ! @ 1\n"
        "    2 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(expression_statements_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "42;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 42\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(comparisons_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "1 > 2; 3 >= 5; 7 == 11; 13 != 17; 19 <= 23; 29 < 31;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
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
        "   11 : 31\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 1 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 2 @ 1\n"
        "    4 : 0d       GREATER                 ; > @ 1\n"
        "    5 : 0b       POP                     ; ; @ 1\n"

        "    6 : 00 02    CONSTANT [2]            ; 3 @ 1\n"
        "    8 : 00 03    CONSTANT [3]            ; 5 @ 1\n"
        "   10 : 0c       LESS                    ; >= @ 1\n"
        "   11 : 0a       NOT                     ; >= @ 1\n"
        "   12 : 0b       POP                     ; ; @ 1\n"

        "   13 : 00 04    CONSTANT [4]            ; 7 @ 1\n"
        "   15 : 00 05    CONSTANT [5]            ; 11 @ 1\n"
        "   17 : 0e       EQUAL                   ; == @ 1\n"
        "   18 : 0b       POP                     ; ; @ 1\n"

        "   19 : 00 06    CONSTANT [6]            ; 13 @ 1\n"
        "   21 : 00 07    CONSTANT [7]            ; 17 @ 1\n"
        "   23 : 0e       EQUAL                   ; != @ 1\n"
        "   24 : 0a       NOT                     ; != @ 1\n"
        "   25 : 0b       POP                     ; ; @ 1\n"

        "   26 : 00 08    CONSTANT [8]            ; 19 @ 1\n"
        "   28 : 00 09    CONSTANT [9]            ; 23 @ 1\n"
        "   30 : 0d       GREATER                 ; <= @ 1\n"
        "   31 : 0a       NOT                     ; <= @ 1\n"
        "   32 : 0b       POP                     ; ; @ 1\n"

        "   33 : 00 0a    CONSTANT [10]           ; 29 @ 1\n"
        "   35 : 00 0b    CONSTANT [11]           ; 31 @ 1\n"
        "   37 : 0c       LESS                    ; < @ 1\n"
        "   38 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(boolean_and_with_short_circuit_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "true and false;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; true @ 1\n"
        "    1 : 0f 00 02 JUMP_IF_FALSE +2 -> 6   ; and @ 1\n"
        "    4 : 0b       POP                     ; and @ 1\n"
        "    5 : 03       FALSE                   ; false @ 1\n"
        "    6 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(boolean_or_with_short_circuit_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "true or false;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; true @ 1\n"
        "    1 : 0f 00 03 JUMP_IF_FALSE +3 -> 7   ; or @ 1\n"
        "    4 : 10 00 02 JUMP +2 -> 9            ; or @ 1\n"
        "    7 : 0b       POP                     ; or @ 1\n"
        "    8 : 03       FALSE                   ; false @ 1\n"
        "    9 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(global_assignment_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "x = 42;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 42\n"
        "    1 : x\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 11 01    SET_GLOBAL [1]          ; = @ 1\n"
        "    4 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(global_identifier_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "x;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : x\n"
        "Bytecode:\n"
        "    0 : 12 00    GET_GLOBAL [0]          ; x @ 1\n"
        "    2 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(while_loop_will_compile) {
    motts::lox::GC_heap gc_heap;

    const auto chunk_loop_expr = compile(gc_heap, "x = 42; while (x > 0) x = x - 1;");
    std::ostringstream os_loop_expr;
    os_loop_expr << chunk_loop_expr;

    const auto chunk_loop_block = compile(gc_heap, "x = 42; while (x > 0) { x = x - 1; }");
    std::ostringstream os_loop_block;
    os_loop_block << chunk_loop_block;

    const auto expected =
        "Constants:\n"
        "    0 : 42\n"
        "    1 : x\n"
        "    2 : 0\n"
        "    3 : 1\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 11 01    SET_GLOBAL [1]          ; = @ 1\n"
        "    4 : 0b       POP                     ; ; @ 1\n"
        "    5 : 12 01    GET_GLOBAL [1]          ; x @ 1\n"
        "    7 : 00 02    CONSTANT [2]            ; 0 @ 1\n"
        "    9 : 0d       GREATER                 ; > @ 1\n"
        "   10 : 0f 00 0c JUMP_IF_FALSE +12 -> 25 ; while @ 1\n"
        "   13 : 0b       POP                     ; while @ 1\n"
        "   14 : 12 01    GET_GLOBAL [1]          ; x @ 1\n"
        "   16 : 00 03    CONSTANT [3]            ; 1 @ 1\n"
        "   18 : 06       SUBTRACT                ; - @ 1\n"
        "   19 : 11 01    SET_GLOBAL [1]          ; = @ 1\n"
        "   21 : 0b       POP                     ; ; @ 1\n"
        "   22 : 13 00 14 LOOP -20 -> 5           ; while @ 1\n"
        "   25 : 0b       POP                     ; while @ 1\n";
    BOOST_TEST(os_loop_expr.str() == expected);
    BOOST_TEST(os_loop_block.str() == expected);
}

BOOST_AUTO_TEST_CASE(blocks_as_statements_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "{ 42; }");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 42\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(var_declaration_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "var x;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : x\n"
        "Bytecode:\n"
        "    0 : 01       NIL                     ; var @ 1\n"
        "    1 : 14 00    DEFINE_GLOBAL [0]       ; var @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(var_declarations_can_be_initialized) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "var x = 42;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 42\n"
        "    1 : x\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 14 01    DEFINE_GLOBAL [1]       ; var @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(vars_will_be_local_inside_braces) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "var x; { var x; x; x = 42; } x;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : x\n"
        "    1 : 42\n"
        "Bytecode:\n"
        "    0 : 01       NIL                     ; var @ 1\n"
        "    1 : 14 00    DEFINE_GLOBAL [0]       ; var @ 1\n"
        "    3 : 01       NIL                     ; var @ 1\n"
        "    4 : 15 00    GET_LOCAL [0]           ; x @ 1\n"
        "    6 : 0b       POP                     ; ; @ 1\n"
        "    7 : 00 01    CONSTANT [1]            ; 42 @ 1\n"
        "    9 : 16 00    SET_LOCAL [0]           ; x @ 1\n"
        "   11 : 0b       POP                     ; ; @ 1\n"
        "   12 : 0b       POP                     ; } @ 1\n"
        "   13 : 12 00    GET_GLOBAL [0]          ; x @ 1\n"
        "   15 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(redeclared_local_vars_will_throw) {
    motts::lox::GC_heap gc_heap;
    BOOST_CHECK_THROW(compile(gc_heap, "{ var x; var x; }"), std::exception);

    try {
        compile(gc_heap, "{ var x; var x; }");
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"x\": Variable with this name already declared in this scope.");
    }
}

BOOST_AUTO_TEST_CASE(using_local_var_in_own_initializer_will_throw) {
    motts::lox::GC_heap gc_heap;
    BOOST_CHECK_THROW(compile(gc_heap, "{ var x = x; }"), std::exception);

    try {
        compile(gc_heap, "{ var x = x; }");
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error at \"x\": Cannot read local variable in its own initializer.");
    }
}

BOOST_AUTO_TEST_CASE(if_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "if (true) nil;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; true @ 1\n"
        "    1 : 0f 00 03 JUMP_IF_FALSE +3 -> 7   ; if @ 1\n"
        "    4 : 0b       POP                     ; if @ 1\n"
        "    5 : 01       NIL                     ; nil @ 1\n"
        "    6 : 0b       POP                     ; ; @ 1\n"
        "    7 : 0b       POP                     ; if @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(if_else_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "if (true) nil; else nil;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; true @ 1\n"
        "    1 : 0f 00 06 JUMP_IF_FALSE +6 -> 10  ; if @ 1\n"
        "    4 : 0b       POP                     ; if @ 1\n"
        "    5 : 01       NIL                     ; nil @ 1\n"
        "    6 : 0b       POP                     ; ; @ 1\n"
        "    7 : 10 00 03 JUMP +3 -> 13           ; else @ 1\n"
        "   10 : 0b       POP                     ; if @ 1\n"
        "   11 : 01       NIL                     ; nil @ 1\n"
        "   12 : 0b       POP                     ; ; @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(for_loops_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "for (var x = 0; x != 3; x = x + 1) nil;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 0\n"
        "    1 : 3\n"
        "    2 : 1\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 0 @ 1\n"
        "    2 : 15 00    GET_LOCAL [0]           ; x @ 1\n"
        "    4 : 00 01    CONSTANT [1]            ; 3 @ 1\n"
        "    6 : 0e       EQUAL                   ; != @ 1\n"
        "    7 : 0a       NOT                     ; != @ 1\n"
        "    8 : 0f 00 14 JUMP_IF_FALSE +20 -> 31 ; for @ 1\n"
        "   11 : 10 00 0b JUMP +11 -> 25          ; for @ 1\n"
        "   14 : 15 00    GET_LOCAL [0]           ; x @ 1\n"
        "   16 : 00 02    CONSTANT [2]            ; 1 @ 1\n"
        "   18 : 04       ADD                     ; + @ 1\n"
        "   19 : 16 00    SET_LOCAL [0]           ; x @ 1\n"
        "   21 : 0b       POP                     ; for @ 1\n"
        "   22 : 13 00 17 LOOP -23 -> 2           ; for @ 1\n"
        "   25 : 0b       POP                     ; for @ 1\n"
        "   26 : 01       NIL                     ; nil @ 1\n"
        "   27 : 0b       POP                     ; ; @ 1\n"
        "   28 : 13 00 11 LOOP -17 -> 14          ; for @ 1\n"
        "   31 : 0b       POP                     ; for @ 1\n"
        "   32 : 0b       POP                     ; for @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(for_loop_init_condition_increment_can_be_blank) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "for (;;) nil;");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 02       TRUE                    ; for @ 1\n"
        "    1 : 0f 00 0c JUMP_IF_FALSE +12 -> 16 ; for @ 1\n"
        "    4 : 10 00 03 JUMP +3 -> 10           ; for @ 1\n"
        "    7 : 13 00 0a LOOP -10 -> 0           ; for @ 1\n"
        "   10 : 0b       POP                     ; for @ 1\n"
        "   11 : 01       NIL                     ; nil @ 1\n"
        "   12 : 0b       POP                     ; ; @ 1\n"
        "   13 : 13 00 09 LOOP -9 -> 7            ; for @ 1\n"
        "   16 : 0b       POP                     ; for @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(for_loop_vars_will_be_local) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(gc_heap, "{ var x = 42; for (var x = 0; x != 3; x = x + 1) nil; }");

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 42\n"
        "    1 : 0\n"
        "    2 : 3\n"
        "    3 : 1\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 1\n"
        "    2 : 00 01    CONSTANT [1]            ; 0 @ 1\n"
        "    4 : 15 01    GET_LOCAL [1]           ; x @ 1\n"
        "    6 : 00 02    CONSTANT [2]            ; 3 @ 1\n"
        "    8 : 0e       EQUAL                   ; != @ 1\n"
        "    9 : 0a       NOT                     ; != @ 1\n"
        "   10 : 0f 00 14 JUMP_IF_FALSE +20 -> 33 ; for @ 1\n"
        "   13 : 10 00 0b JUMP +11 -> 27          ; for @ 1\n"
        "   16 : 15 01    GET_LOCAL [1]           ; x @ 1\n"
        "   18 : 00 03    CONSTANT [3]            ; 1 @ 1\n"
        "   20 : 04       ADD                     ; + @ 1\n"
        "   21 : 16 01    SET_LOCAL [1]           ; x @ 1\n"
        "   23 : 0b       POP                     ; for @ 1\n"
        "   24 : 13 00 17 LOOP -23 -> 4           ; for @ 1\n"
        "   27 : 0b       POP                     ; for @ 1\n"
        "   28 : 01       NIL                     ; nil @ 1\n"
        "   29 : 0b       POP                     ; ; @ 1\n"
        "   30 : 13 00 11 LOOP -17 -> 16          ; for @ 1\n"
        "   33 : 0b       POP                     ; for @ 1\n"
        "   34 : 0b       POP                     ; for @ 1\n"
        "   35 : 0b       POP                     ; } @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(non_var_statements_in_for_loop_init_will_throw) {
    motts::lox::GC_heap gc_heap;
    BOOST_CHECK_THROW(compile(gc_heap, "for (print x;;) nil;"), std::exception);

    try {
        compile(gc_heap, "for (print x;;)");
    } catch (const std::exception& error) {
        BOOST_TEST(error.what() == "[Line 1] Error: Unexpected token \"print\".");
    }
}

BOOST_AUTO_TEST_CASE(function_declaration_and_invocation_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(
        gc_heap,
        "fun f() {}\n"
        "f();\n"
        "{ fun g() {} }\n"
    );

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : f\n"
        "    2 : <fn g>\n"
        "Bytecode:\n"
        "    0 : 19 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 14 01    DEFINE_GLOBAL [1]       ; fun @ 1\n"
        "    5 : 12 01    GET_GLOBAL [1]          ; f @ 2\n"
        "    7 : 17 00    CALL [0]                ; f @ 2\n"
        "    9 : 0b       POP                     ; ; @ 2\n"
        "   10 : 19 02 00 CLOSURE [2] (0)         ; fun @ 3\n"
        "   13 : 0b       POP                     ; } @ 3\n"
        "## <fn f> chunk\n"
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 01       NIL                     ; fun @ 1\n"
        "    1 : 18       RETURN                  ; fun @ 1\n"
        "## <fn g> chunk\n"
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 01       NIL                     ; fun @ 3\n"
        "    1 : 18       RETURN                  ; fun @ 3\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(function_parameters_arguments_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(
        gc_heap,
        "fun f(x) { x; }\n"
        "f(42);\n"
    );

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : f\n"
        "    2 : 42\n"
        "Bytecode:\n"
        "    0 : 19 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 14 01    DEFINE_GLOBAL [1]       ; fun @ 1\n"
        "    5 : 12 01    GET_GLOBAL [1]          ; f @ 2\n"
        "    7 : 00 02    CONSTANT [2]            ; 42 @ 2\n"
        "    9 : 17 01    CALL [1]                ; f @ 2\n"
        "   11 : 0b       POP                     ; ; @ 2\n"
        "## <fn f> chunk\n"
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 15 01    GET_LOCAL [1]           ; x @ 1\n"
        "    2 : 0b       POP                     ; ; @ 1\n"
        "    3 : 01       NIL                     ; fun @ 1\n"
        "    4 : 18       RETURN                  ; fun @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(function_return_will_compile) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(
        gc_heap,
        "fun f(x) { return x; }\n"
        "f(42);\n"
    );

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : f\n"
        "    2 : 42\n"
        "Bytecode:\n"
        "    0 : 19 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 14 01    DEFINE_GLOBAL [1]       ; fun @ 1\n"
        "    5 : 12 01    GET_GLOBAL [1]          ; f @ 2\n"
        "    7 : 00 02    CONSTANT [2]            ; 42 @ 2\n"
        "    9 : 17 01    CALL [1]                ; f @ 2\n"
        "   11 : 0b       POP                     ; ; @ 2\n"
        "## <fn f> chunk\n"
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 15 01    GET_LOCAL [1]           ; x @ 1\n"
        "    2 : 18       RETURN                  ; return @ 1\n"
        "    3 : 01       NIL                     ; fun @ 1\n"
        "    4 : 18       RETURN                  ; fun @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(empty_return_will_return_nil) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(
        gc_heap,
        "fun f() { return; }\n"
        "f();\n"
    );

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : f\n"
        "Bytecode:\n"
        "    0 : 19 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 14 01    DEFINE_GLOBAL [1]       ; fun @ 1\n"
        "    5 : 12 01    GET_GLOBAL [1]          ; f @ 2\n"
        "    7 : 17 00    CALL [0]                ; f @ 2\n"
        "    9 : 0b       POP                     ; ; @ 2\n"
        "## <fn f> chunk\n"
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 01       NIL                     ; return @ 1\n"
        "    1 : 18       RETURN                  ; return @ 1\n"
        "    2 : 01       NIL                     ; fun @ 1\n"
        "    3 : 18       RETURN                  ; fun @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(function_body_will_have_local_access_to_original_function_name) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(
        gc_heap,
        "fun f() { f; }\n"
    );

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : <fn f>\n"
        "    1 : f\n"
        "Bytecode:\n"
        "    0 : 19 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 14 01    DEFINE_GLOBAL [1]       ; fun @ 1\n"
        "## <fn f> chunk\n"
        "Constants:\n"
        "Bytecode:\n"
        "    0 : 15 00    GET_LOCAL [0]           ; f @ 1\n"
        "    2 : 0b       POP                     ; ; @ 1\n"
        "    3 : 01       NIL                     ; fun @ 1\n"
        "    4 : 18       RETURN                  ; fun @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(inner_functions_can_capture_enclosing_local_variables) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(
        gc_heap,
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

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : <fn outer>\n"
        "    1 : outer\n"
        "Bytecode:\n"
        "    0 : 19 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 14 01    DEFINE_GLOBAL [1]       ; fun @ 1\n"

        "## <fn outer> chunk\n"
        "Constants:\n"
        "    0 : 42\n"
        "    1 : <fn middle>\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 2\n"
        "    2 : 19 01 01 CLOSURE [1] (1)         ; fun @ 3\n"
        "           01 01 | ^ [1]                 ; fun @ 3\n"
        "    7 : 15 02    GET_LOCAL [2]           ; middle @ 10\n"
        "    9 : 17 00    CALL [0]                ; middle @ 10\n"
        "   11 : 0b       POP                     ; ; @ 10\n"
        "   12 : 0b       POP                     ; } @ 11\n"
        "   13 : 1c       CLOSE_UPVALUE           ; } @ 11\n"
        "   14 : 01       NIL                     ; fun @ 1\n"
        "   15 : 18       RETURN                  ; fun @ 1\n"

        "## <fn middle> chunk\n"
        "Constants:\n"
        "    0 : <fn inner>\n"
        "Bytecode:\n"
        "    0 : 19 00 01 CLOSURE [0] (1)         ; fun @ 4\n"
        "           00 00 | ^^ [0]                ; fun @ 4\n"
        "    5 : 15 01    GET_LOCAL [1]           ; inner @ 8\n"
        "    7 : 17 00    CALL [0]                ; inner @ 8\n"
        "    9 : 0b       POP                     ; ; @ 8\n"
        "   10 : 0b       POP                     ; } @ 9\n"
        "   11 : 01       NIL                     ; fun @ 3\n"
        "   12 : 18       RETURN                  ; fun @ 3\n"

        "## <fn inner> chunk\n"
        "Constants:\n"
        "    0 : 108\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 108 @ 5\n"
        "    2 : 1b 00    SET_UPVALUE [0]         ; x @ 5\n"
        "    4 : 0b       POP                     ; ; @ 5\n"
        "    5 : 1a 00    GET_UPVALUE [0]         ; x @ 6\n"
        "    7 : 05       PRINT                   ; print @ 6\n"
        "    8 : 01       NIL                     ; fun @ 4\n"
        "    9 : 18       RETURN                  ; fun @ 4\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(closed_variables_stay_alive_even_after_function_has_returned) {
    motts::lox::GC_heap gc_heap;
    const auto chunk = compile(
        gc_heap,
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

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : <fn outer>\n"
        "    1 : outer\n"
        "    2 : closure\n"
        "Bytecode:\n"
        "    0 : 19 00 00 CLOSURE [0] (0)         ; fun @ 1\n"
        "    3 : 14 01    DEFINE_GLOBAL [1]       ; fun @ 1\n"
        "    5 : 12 01    GET_GLOBAL [1]          ; outer @ 12\n"
        "    7 : 17 00    CALL [0]                ; outer @ 12\n"
        "    9 : 14 02    DEFINE_GLOBAL [2]       ; var @ 12\n"
        "   11 : 12 02    GET_GLOBAL [2]          ; closure @ 13\n"
        "   13 : 17 00    CALL [0]                ; closure @ 13\n"
        "   15 : 0b       POP                     ; ; @ 13\n"

        "## <fn outer> chunk\n"
        "Constants:\n"
        "    0 : 42\n"
        "    1 : <fn middle>\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 42 @ 2\n"
        "    2 : 19 01 01 CLOSURE [1] (1)         ; fun @ 3\n"
        "           01 01 | ^ [1]                 ; fun @ 3\n"
        "    7 : 15 02    GET_LOCAL [2]           ; middle @ 10\n"
        "    9 : 18       RETURN                  ; return @ 10\n"
        "   10 : 0b       POP                     ; } @ 11\n"
        "   11 : 1c       CLOSE_UPVALUE           ; } @ 11\n"
        "   12 : 01       NIL                     ; fun @ 1\n"
        "   13 : 18       RETURN                  ; fun @ 1\n"

        "## <fn middle> chunk\n"
        "Constants:\n"
        "    0 : <fn inner>\n"
        "Bytecode:\n"
        "    0 : 19 00 01 CLOSURE [0] (1)         ; fun @ 4\n"
        "           00 00 | ^^ [0]                ; fun @ 4\n"
        "    5 : 15 01    GET_LOCAL [1]           ; inner @ 8\n"
        "    7 : 17 00    CALL [0]                ; inner @ 8\n"
        "    9 : 0b       POP                     ; ; @ 8\n"
        "   10 : 0b       POP                     ; } @ 9\n"
        "   11 : 01       NIL                     ; fun @ 3\n"
        "   12 : 18       RETURN                  ; fun @ 3\n"

        "## <fn inner> chunk\n"
        "Constants:\n"
        "    0 : 108\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; 108 @ 5\n"
        "    2 : 1b 00    SET_UPVALUE [0]         ; x @ 5\n"
        "    4 : 0b       POP                     ; ; @ 5\n"
        "    5 : 1a 00    GET_UPVALUE [0]         ; x @ 6\n"
        "    7 : 05       PRINT                   ; print @ 6\n"
        "    8 : 01       NIL                     ; fun @ 4\n"
        "    9 : 18       RETURN                  ; fun @ 4\n";
    BOOST_TEST(os.str() == expected);
}
