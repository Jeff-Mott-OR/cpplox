#define BOOST_TEST_MODULE CppLox Tests
#include <boost/test/unit_test.hpp>

#include <sstream>
#include <stdexcept>

#include "scanner.hpp"

BOOST_AUTO_TEST_CASE(token_types_can_be_printed) {
    {
        std::ostringstream os;
        os << motts::lox::Token_type::number;
        BOOST_TEST(os.str() == "NUMBER");
    }
    {
        std::ostringstream os;
        os << motts::lox::Token_type::and_;
        BOOST_TEST(os.str() == "AND");
    }
}

BOOST_AUTO_TEST_CASE(printing_invalid_token_types_will_throw) {
    std::ostringstream os;
    const auto invalid_token_type_value = 276'709;
    const auto invalid_token_type = reinterpret_cast<const motts::lox::Token_type&>(invalid_token_type_value);
    BOOST_CHECK_THROW(os << invalid_token_type, std::exception);
}
