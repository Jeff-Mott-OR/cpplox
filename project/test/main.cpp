#define BOOST_TEST_MODULE CppLox Test

#include <iterator>
#include <string>

#include <boost/algorithm/string/replace.hpp>
#include <boost/predef.h>
#include <boost/process.hpp>
#include <boost/test/unit_test.hpp>
#include <gsl/span>

using std::istreambuf_iterator;
using std::string;

using boost::process::ipstream;
using boost::process::std_out;
using boost::process::system;
using boost::replace_all;
using boost::unit_test::framework::master_test_suite;
using gsl::span;

const char* main_exe_path() {
    // TODO: Use Program_options here
    static span<char*> argv_span {master_test_suite().argv, master_test_suite().argc};

    return argv_span.at(1);
}

BOOST_AUTO_TEST_CASE(identifiers_test) {
    const auto expected = "(+ (group (- 5.0 (group (- 3.0 1.0)))) (- 1.0))\n";

    ipstream cpplox_out;
    const auto exit_code = system(main_exe_path(), "scripts/parse.lox", std_out > cpplox_out);
    string actual {istreambuf_iterator<char>{cpplox_out}, istreambuf_iterator<char>{}};

    // Reading from the pipe stream doesn't convert newlines, so we'll have to do that ourselves
    if (BOOST_OS_WINDOWS) {
        replace_all(actual, "\r\n", "\n");
    }

    BOOST_TEST(actual == expected);
    BOOST_TEST(exit_code == 0);
}
