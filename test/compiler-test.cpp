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
    chunk.constants.push_back(276'709);
    chunk.bytecode.push_back(0); // opcode constant
    chunk.bytecode.push_back(0); // constant index
    chunk.source_map_tokens.push_back(motts::lox::Token{motts::lox::Token_type::number, "276709", 1}); // for bytecode[0]
    chunk.source_map_tokens.push_back(motts::lox::Token{motts::lox::Token_type::number, "276709", 1}); // for bytecode[1]

    std::ostringstream os;
    os << chunk;

    const auto expected =
        "Constants:\n"
        "    0 : 276709\n"
        "Bytecode:\n"
        "    0 : 00 00 CONSTANT [0] ; 276709 @ 1\n";
    BOOST_TEST(os.str() == expected);
}

BOOST_AUTO_TEST_CASE(printing_invalid_chunk_opcode_will_throw, *boost::unit_test::timeout(2)) {
    motts::lox::Chunk invalid_chunk;
    invalid_chunk.constants.push_back(276'709);
    invalid_chunk.bytecode.push_back(255); // invalid opcode constant
    invalid_chunk.bytecode.push_back(0);
    invalid_chunk.source_map_tokens.push_back(motts::lox::Token{motts::lox::Token_type::number, "276709", 1});
    invalid_chunk.source_map_tokens.push_back(motts::lox::Token{motts::lox::Token_type::number, "276709", 1});

    std::ostringstream os;
    BOOST_CHECK_THROW(os << invalid_chunk, std::exception);
}

BOOST_AUTO_TEST_CASE(printing_invalid_chunk_bytecode_will_throw, *boost::unit_test::timeout(2)) {
    motts::lox::Chunk invalid_chunk;
    invalid_chunk.constants.push_back(276'709);
    invalid_chunk.bytecode.push_back(0); // opcode constant, missing constant index
    invalid_chunk.source_map_tokens.push_back(motts::lox::Token{motts::lox::Token_type::number, "276709", 1});
    invalid_chunk.source_map_tokens.push_back(motts::lox::Token{motts::lox::Token_type::number, "276709", 1});

    std::ostringstream os;
    BOOST_CHECK_THROW(os << invalid_chunk, std::exception);
}
