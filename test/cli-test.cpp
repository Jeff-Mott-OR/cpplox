#define BOOST_TEST_MODULE CLI Tests
#include <boost/test/unit_test.hpp>

#include <iterator>
#include <string>

#include <boost/process.hpp>

BOOST_AUTO_TEST_CASE(script_file_as_positional_argument_will_run)
{
    boost::process::ipstream cpplox_out;
    boost::process::ipstream cpplox_err;
    const auto exit_code = boost::process::system(
        "cpploxbc ../src/test/lox/hello.lox",
        boost::process::std_out > cpplox_out,
        boost::process::std_err > cpplox_err
    );
    std::string actual_out {std::istreambuf_iterator<char>{cpplox_out}, {}};
    std::string actual_err {std::istreambuf_iterator<char>{cpplox_err}, {}};

    BOOST_TEST(actual_out == "Hello, World!\n");
    BOOST_TEST(actual_err == "");
    BOOST_TEST(exit_code == 0);
}

BOOST_AUTO_TEST_CASE(debug_option_will_print_bytecode_and_stack)
{
    boost::process::ipstream cpplox_out;
    boost::process::ipstream cpplox_err;
    const auto exit_code = boost::process::system(
        "cpploxbc ../src/test/lox/hello.lox --debug",
        boost::process::std_out > cpplox_out,
        boost::process::std_err > cpplox_err
    );
    std::string actual_out {std::istreambuf_iterator<char>{cpplox_out}, {}};
    std::string actual_err {std::istreambuf_iterator<char>{cpplox_err}, {}};

    const auto expected =
        "\n"
        "# Running chunk:\n"
        "\n"
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

BOOST_AUTO_TEST_CASE(invalid_syntax_will_print_to_stderr_and_set_exit_code)
{
    boost::process::ipstream cpplox_out;
    boost::process::ipstream cpplox_err;
    const auto exit_code = boost::process::system(
        "cpploxbc ../src/test/lox/invalid_syntax.lox",
        boost::process::std_out > cpplox_out,
        boost::process::std_err > cpplox_err
    );
    std::string actual_out {std::istreambuf_iterator<char>{cpplox_out}, {}};
    std::string actual_err {std::istreambuf_iterator<char>{cpplox_err}, {}};

    BOOST_TEST(actual_out == "");
    BOOST_TEST(actual_err == "[Line 1] Error: Unexpected character \"?\".\n");
    BOOST_TEST(exit_code == 1);
}

BOOST_AUTO_TEST_CASE(no_script_file_will_start_interactive_repl)
{
    boost::process::ipstream cpplox_out;
    boost::process::ipstream cpplox_err;
    boost::process::opstream cpplox_in;
    const auto child_process = boost::process::child(
        "cpploxbc",
        boost::process::std_out > cpplox_out,
        boost::process::std_err > cpplox_err,
        boost::process::std_in < cpplox_in
    );
    std::string line;

    cpplox_out >> line;
    BOOST_TEST(line == ">");

    cpplox_in << "var x = 42;\n" << std::flush;
    cpplox_out >> line;
    BOOST_TEST(line == ">");

    cpplox_in << "print x;\n" << std::flush;
    cpplox_out >> line;
    BOOST_TEST(line == "42");

    cpplox_out >> line;
    BOOST_TEST(line == ">");
}

BOOST_AUTO_TEST_CASE(can_print_class_name_across_interactive_repl_runs)
{
    boost::process::ipstream cpplox_out;
    boost::process::ipstream cpplox_err;
    boost::process::opstream cpplox_in;
    const auto child_process = boost::process::child(
        "cpploxbc",
        boost::process::std_out > cpplox_out,
        boost::process::std_err > cpplox_err,
        boost::process::std_in < cpplox_in
    );
    std::string line;

    cpplox_out >> line;
    BOOST_TEST(line == ">");

    cpplox_in << "class Foo {}\n" << std::flush;
    cpplox_out >> line;
    BOOST_TEST(line == ">");

    cpplox_in << "print Foo;\n" << std::flush;
    std::getline(cpplox_out >> std::ws, line);
    BOOST_TEST(line == "<class Foo>");

    cpplox_out >> line;
    BOOST_TEST(line == ">");
}
