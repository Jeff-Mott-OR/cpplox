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

using boost::process::ipstream;
using boost::process::std_out;
using boost::process::system;
using boost::replace_all;

// Boost.Test defines main for us, so all variables related to program options needs to be either
// globals or statics inside accessors.
const auto& program_options_description() {
    using boost::program_options::options_description;
    using boost::program_options::value;

    static unique_ptr<options_description> options_description_;

    if (!options_description_) {
        options_description_ = make_unique<options_description>(
            "Usage: test_harness -- [options]\n"
            "\n"
            "Options"
        );
        options_description_->add_options()
            ("help", "Print usage information and exit.")
            ("cpplox-file", value<string>(), "Required. File path to cpplox executable.")
            ("test-scripts-path", value<string>(), "Required. Path to test scripts.");
    }

    return *options_description_;
}

const auto& program_options_map() {
    using boost::program_options::notify;
    using boost::program_options::parse_command_line;
    using boost::program_options::store;
    using boost::program_options::variables_map;
    using boost::unit_test::framework::master_test_suite;

    static unique_ptr<variables_map> variables_map_;

    if (!variables_map_) {
        variables_map_ = make_unique<variables_map>();
        store(
            parse_command_line(
                master_test_suite().argc,
                master_test_suite().argv,
                program_options_description()
            ),
            *variables_map_
        );
        notify(*variables_map_);

        // Asking for help trumps all else
        if (variables_map_->count("help")) {
            cout << program_options_description() << "\n";

            exit(EXIT_FAILURE);
        }
    }

    return *variables_map_;
}

// A helper function so users can write `program_option("name")` rather than
// `program_options_map().at("name").as<string>()`.
template<typename T = string>
    auto program_option(const string& key) {
        if (!program_options_map().count(key)) {
            cout << program_options_description() << "\n";

            exit(EXIT_FAILURE);
        }

        return program_options_map().at(key).as<T>();
    }

BOOST_AUTO_TEST_CASE(evaluate_test) {
    const auto expected = "2\n";

    ipstream cpplox_out;
    const auto exit_code = system(program_option("cpplox-file"), program_option("test-scripts-path") + "/evaluate.lox", std_out > cpplox_out);
    string actual {istreambuf_iterator<char>{cpplox_out}, istreambuf_iterator<char>{}};

    // Reading from the pipe stream doesn't convert newlines, so we'll have to do that ourselves
    if (BOOST_OS_WINDOWS) {
        replace_all(actual, "\r\n", "\n");
    }

    BOOST_TEST(actual == expected);
    BOOST_TEST(exit_code == 0);
}
