#define BOOST_TEST_MODULE CppLox Test

#include <cstdlib>

#include <iostream>
#include <iterator>
#include <memory>
#include <string>

#include <boost/algorithm/string/replace.hpp>
#include <boost/predef.h>
#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <boost/test/unit_test.hpp>

using std::cout;
using std::exit;
using std::istreambuf_iterator;
using std::make_unique;
using std::string;
using std::unique_ptr;

namespace process = boost::process;
namespace program_options = boost::program_options;
using boost::replace_all;
namespace unit_test = boost::unit_test::framework;

// Boost.Test defines main for us, so all variables related to program options need to be either global or static inside accessors
const auto& program_options_description() {
    static unique_ptr<program_options::options_description> options_description;

    if (!options_description) {
        options_description = make_unique<program_options::options_description>(
            "Usage: test_harness -- [options]\n"
            "\n"
            "Options"
        );
        options_description->add_options()
            ("help", "Print usage information and exit.")
            ("cpplox-file", program_options::value<string>(), "Required. File path to cpplox executable.")
            ("test-scripts-path", program_options::value<string>(), "Required. Path to test scripts.");
    }

    return *options_description;
}

const auto& program_options_map() {
    static unique_ptr<program_options::variables_map> variables_map;

    if (!variables_map) {
        variables_map = make_unique<program_options::variables_map>();
        program_options::store(
            program_options::parse_command_line(
                unit_test::master_test_suite().argc,
                unit_test::master_test_suite().argv,
                program_options_description()
            ),
            *variables_map
        );
        program_options::notify(*variables_map);

        // Check required options or respond to help
        if (
            variables_map->count("help") ||
            !variables_map->count("cpplox-file") ||
            !variables_map->count("test-scripts-path")
        ) {
            cout << program_options_description() << "\n";
            exit(EXIT_FAILURE);
        }
    }

    return *variables_map;
}

BOOST_AUTO_TEST_CASE(evaluate_test) {
    const auto expected = "2\n";

    process::ipstream cpplox_out;
    const auto exit_code = process::system(
        program_options_map().at("cpplox-file").as<string>(),
        program_options_map().at("test-scripts-path").as<string>() + "/evaluate.lox",
        process::std_out > cpplox_out
    );
    string actual {istreambuf_iterator<char>{cpplox_out}, istreambuf_iterator<char>{}};

    // Reading from the pipe stream doesn't convert newlines, so we'll have to do that ourselves
    if (BOOST_OS_WINDOWS) {
        replace_all(actual, "\r\n", "\n");
    }

    BOOST_TEST(actual == expected);
    BOOST_TEST(exit_code == 0);
}
