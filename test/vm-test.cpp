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
