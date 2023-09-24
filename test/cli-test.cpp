#define BOOST_TEST_MODULE CLI Tests
#include <boost/test/unit_test.hpp>

#include <iterator>
#include <string>

#include <boost/process.hpp>

namespace process = boost::process;

BOOST_AUTO_TEST_CASE(script_file_as_positional_argument_will_run) {
    process::ipstream cpplox_out;
    process::ipstream cpplox_err;

    const auto exit_code = process::system(
        "cpploxbc ../test/lox/hello.lox",
        process::std_out > cpplox_out,
        process::std_err > cpplox_err
    );

    std::string actual_out {std::istreambuf_iterator<char>{cpplox_out}, std::istreambuf_iterator<char>{}};
    std::string actual_err {std::istreambuf_iterator<char>{cpplox_err}, std::istreambuf_iterator<char>{}};

    BOOST_TEST(actual_out == "Hello, World!\n");
    BOOST_TEST(actual_err == "");
    BOOST_TEST(exit_code == 0);
}

BOOST_AUTO_TEST_CASE(debug_option_will_print_bytecode_and_stack) {
    process::ipstream cpplox_out;
    process::ipstream cpplox_err;

    const auto exit_code = process::system(
        "cpploxbc ../test/lox/hello.lox --debug",
        process::std_out > cpplox_out,
        process::std_err > cpplox_err
    );

    std::string actual_out {std::istreambuf_iterator<char>{cpplox_out}, std::istreambuf_iterator<char>{}};
    std::string actual_err {std::istreambuf_iterator<char>{cpplox_err}, std::istreambuf_iterator<char>{}};

    const auto expected =
        "# Running chunk:\n"
        "Constants:\n"
        "    0 : Hello, World!\n"
        "Bytecode:\n"
        "    0 : 00 00    CONSTANT [0]            ; \"Hello, World!\" @ 1\n"
        "    2 : 05       PRINT                   ; print @ 1\n"
        "\n"
        "Stack:\n"
        "    0 : Hello, World!\n"
        "\n"
        "Hello, World!\n"
        "Stack:\n"
        "\n";

    BOOST_TEST(actual_out == expected);
    BOOST_TEST(actual_err == "");
    BOOST_TEST(exit_code == 0);
}

BOOST_AUTO_TEST_CASE(invalid_syntax_will_print_to_stderr_and_set_exit_code) {
    process::ipstream cpplox_out;
    process::ipstream cpplox_err;

    const auto exit_code = process::system(
        "cpploxbc ../test/lox/invalid_syntax.lox",
        process::std_out > cpplox_out,
        process::std_err > cpplox_err
    );

    std::string actual_out {std::istreambuf_iterator<char>{cpplox_out}, std::istreambuf_iterator<char>{}};
    std::string actual_err {std::istreambuf_iterator<char>{cpplox_err}, std::istreambuf_iterator<char>{}};

    BOOST_TEST(actual_out == "");
    BOOST_TEST(actual_err == "[Line 1] Error: Unexpected character \"?\".\n");
    BOOST_TEST(exit_code == 1);

}

BOOST_AUTO_TEST_CASE(no_script_file_will_start_interactive_repl) {
    process::ipstream cpplox_out;
    process::ipstream cpplox_err;
    process::opstream cpplox_in;
    std::string cpplox_out_line;

    auto child_process = process::child(
        "cpploxbc",
        process::std_out > cpplox_out,
        process::std_err > cpplox_err,
        process::std_in < cpplox_in
    );

    cpplox_out >> cpplox_out_line;
    BOOST_TEST(cpplox_out_line == ">");

    cpplox_in << "x = 42;" << std::endl;
    cpplox_out >> cpplox_out_line;
    BOOST_TEST(cpplox_out_line == ">");

    cpplox_in << "print x;" << std::endl;
    cpplox_out >> cpplox_out_line;
    BOOST_TEST(cpplox_out_line == "42");

    cpplox_out >> cpplox_out_line;
    BOOST_TEST(cpplox_out_line == ">");
}
