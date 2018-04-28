#define BOOST_TEST_MODULE CppLox Test

// C standard headers
#include <cstdlib>
// C++ standard headers
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
// Third-party headers
#include <boost/algorithm/string/replace.hpp>
#include <boost/predef.h>
#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <boost/test/unit_test.hpp>
// This project's headers

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

// Boost.Test defines main for us, so all variables related to program options need to be either global or static inside
// accessors.
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

auto expect_script_file_out_to_be(
    const string& script_file,
    const string& expected_out,
    const string& expected_err = "",
    int expected_exit_code = 0
) {
    process::ipstream cpplox_out;
    process::ipstream cpplox_err;
    const auto exit_code = process::system(
        program_options_map().at("cpplox-file").as<string>(),
        program_options_map().at("test-scripts-path").as<string>() + "/" + script_file,
        process::std_out > cpplox_out,
        process::std_err > cpplox_err
    );
    string actual_out {istreambuf_iterator<char>{cpplox_out}, istreambuf_iterator<char>{}};
    string actual_err {istreambuf_iterator<char>{cpplox_err}, istreambuf_iterator<char>{}};

    // Reading from the pipe stream doesn't convert newlines, so we'll have to do that ourselves
    if (BOOST_OS_WINDOWS) {
        replace_all(actual_out, "\r\n", "\n");
        replace_all(actual_err, "\r\n", "\n");
    }

    BOOST_TEST(actual_out == expected_out);
    BOOST_TEST(actual_err == expected_err);
    BOOST_TEST(exit_code == expected_exit_code);
}

BOOST_AUTO_TEST_CASE(assignment_associativity_test) { expect_script_file_out_to_be("assignment/associativity.lox", "c\nc\nc\n"); }
BOOST_AUTO_TEST_CASE(assignment_global_test) { expect_script_file_out_to_be("assignment/global.lox", "before\nafter\narg\narg\n"); }
BOOST_AUTO_TEST_CASE(assignment_grouping_test) { expect_script_file_out_to_be("assignment/grouping.lox", "", "[Line 2] Error at '=': Invalid assignment target.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(assignment_infix_operator_test) { expect_script_file_out_to_be("assignment/infix_operator.lox", "", "[Line 3] Error at '=': Invalid assignment target.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(assignment_local_test) { expect_script_file_out_to_be("assignment/local.lox", "before\nafter\narg\narg\n"); }
BOOST_AUTO_TEST_CASE(assignment_prefix_operator_test) { expect_script_file_out_to_be("assignment/prefix_operator.lox", "", "[Line 2] Error at '=': Invalid assignment target.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(assignment_syntax_test) { expect_script_file_out_to_be("assignment/syntax.lox", "var\nvar\n"); }
BOOST_AUTO_TEST_CASE(assignment_undefined_test) { expect_script_file_out_to_be("assignment/undefined.lox", "", "Undefined variable 'unknown'.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(block_empty_test) { expect_script_file_out_to_be("block/empty.lox", "ok\n"); }
BOOST_AUTO_TEST_CASE(block_scope_test) { expect_script_file_out_to_be("block/scope.lox", "inner\nouter\n"); }

BOOST_AUTO_TEST_CASE(bool_equality_test) { expect_script_file_out_to_be("bool/equality.lox", "true\nfalse\nfalse\ntrue\n" "false\nfalse\nfalse\nfalse\nfalse\n" "false\ntrue\ntrue\nfalse\n" "true\ntrue\ntrue\ntrue\ntrue\n"); }
BOOST_AUTO_TEST_CASE(bool_not_test) { expect_script_file_out_to_be("bool/not.lox", "false\ntrue\ntrue\n"); }

BOOST_AUTO_TEST_CASE(call_bool_test) { expect_script_file_out_to_be("call/bool.lox", "", "Can only call functions and classes.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(call_nil_test) { expect_script_file_out_to_be("call/nil.lox", "", "Can only call functions and classes.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(call_num_test) { expect_script_file_out_to_be("call/num.lox", "", "Can only call functions and classes.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(call_string_test) { expect_script_file_out_to_be("call/string.lox", "", "Can only call functions and classes.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(closure_assign_to_closure_test) { expect_script_file_out_to_be("closure/assign_to_closure.lox", "local\nafter f\nafter f\nafter g\n"); }
BOOST_AUTO_TEST_CASE(closure_assign_to_shadowed_later_test) { expect_script_file_out_to_be("closure/assign_to_shadowed_later.lox", "inner\nassigned\n"); }
BOOST_AUTO_TEST_CASE(closure_close_over_function_parameter_test) { expect_script_file_out_to_be("closure/close_over_function_parameter.lox", "param\n"); }
BOOST_AUTO_TEST_CASE(closure_close_over_later_variable_test) { expect_script_file_out_to_be("closure/close_over_later_variable.lox", "b\na\n"); }
BOOST_AUTO_TEST_CASE(closure_closed_closure_in_function_test) { expect_script_file_out_to_be("closure/closed_closure_in_function.lox", "local\n"); }
BOOST_AUTO_TEST_CASE(closure_nested_closure_test) { expect_script_file_out_to_be("closure/nested_closure.lox", "a\nb\nc\n"); }
BOOST_AUTO_TEST_CASE(closure_open_closure_in_function_test) { expect_script_file_out_to_be("closure/open_closure_in_function.lox", "local\n"); }
BOOST_AUTO_TEST_CASE(closure_reference_closure_multiple_times_test) { expect_script_file_out_to_be("closure/reference_closure_multiple_times.lox", "a\na\n"); }
BOOST_AUTO_TEST_CASE(closure_reuse_closure_slot_test) { expect_script_file_out_to_be("closure/reuse_closure_slot.lox", "a\n"); }
BOOST_AUTO_TEST_CASE(closure_shadow_closure_with_local_test) { expect_script_file_out_to_be("closure/shadow_closure_with_local.lox", "closure\nshadow\nclosure\n"); }
BOOST_AUTO_TEST_CASE(closure_unused_closure_test) { expect_script_file_out_to_be("closure/unused_closure.lox", "ok\n"); }
BOOST_AUTO_TEST_CASE(closure_unused_later_closure_test) { expect_script_file_out_to_be("closure/unused_later_closure.lox", "a\n"); }

BOOST_AUTO_TEST_CASE(comments_line_at_eof_test) { expect_script_file_out_to_be("comments/line_at_eof.lox", "ok\n"); }
BOOST_AUTO_TEST_CASE(comments_only_line_comment_test) { expect_script_file_out_to_be("comments/only_line_comment.lox", ""); }
BOOST_AUTO_TEST_CASE(comments_only_line_comment_and_line_test) { expect_script_file_out_to_be("comments/only_line_comment_and_line.lox", ""); }
BOOST_AUTO_TEST_CASE(comments_unicode_test) { expect_script_file_out_to_be("comments/unicode.lox", "ok\n"); }

BOOST_AUTO_TEST_CASE(for_closure_in_body_test) { expect_script_file_out_to_be("for/closure_in_body.lox", "1\n2\n3\n"); }
BOOST_AUTO_TEST_CASE(for_fun_in_body_test) { expect_script_file_out_to_be("for/fun_in_body.lox", "", "[Line 2] Error at 'fun': Expected expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(for_return_closure_test) { expect_script_file_out_to_be("for/return_closure.lox", "i\n"); }
BOOST_AUTO_TEST_CASE(for_return_inside_test) { expect_script_file_out_to_be("for/return_inside.lox", "i\n"); }
BOOST_AUTO_TEST_CASE(for_scope_test) { expect_script_file_out_to_be("for/scope.lox", "0\n-1\nafter\n0\n"); }
BOOST_AUTO_TEST_CASE(for_statement_condition_test) { expect_script_file_out_to_be("for/statement_condition.lox", "", "[Line 3] Error at '{': Expected expression.\n[Line 3] Error at ')': Expected ';' after expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(for_statement_increment_test) { expect_script_file_out_to_be("for/statement_increment.lox", "", "[Line 2] Error at '{': Expected expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(for_statement_initializer_test) { expect_script_file_out_to_be("for/statement_initializer.lox", "", "[Line 3] Error at '{': Expected expression.\n[Line 3] Error at ')': Expected ';' after expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(for_syntax_test) { expect_script_file_out_to_be("for/syntax.lox", "1\n2\n3\n0\n1\n2\ndone\n0\n1\n0\n1\n2\n0\n1\n"); }
BOOST_AUTO_TEST_CASE(for_var_in_body_test) { expect_script_file_out_to_be("for/var_in_body.lox", "", "[Line 2] Error at 'var': Expected expression.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(function_body_must_be_block_test) { expect_script_file_out_to_be("function/body_must_be_block.lox", "", "[Line 3] Error at '123': Expected '{' before function body.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(function_empty_body_test) { expect_script_file_out_to_be("function/empty_body.lox", "nil\n"); }
BOOST_AUTO_TEST_CASE(function_extra_arguments_test) { expect_script_file_out_to_be("function/extra_arguments.lox", "", "[Line 6] Error ')': Expected 2 arguments but got 4.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(function_local_mutual_recursion_test) { expect_script_file_out_to_be("function/local_mutual_recursion.lox", "", "Undefined variable 'isOdd'.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(function_local_recursion_test) { expect_script_file_out_to_be("function/local_recursion.lox", "21\n"); }
BOOST_AUTO_TEST_CASE(function_missing_arguments_test) { expect_script_file_out_to_be("function/missing_arguments.lox", "", "[Line 3] Error ')': Expected 2 arguments but got 1.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(function_missing_comma_in_parameters_test) { expect_script_file_out_to_be("function/missing_comma_in_parameters.lox", "", "[Line 3] Error at 'c': Expected ')' after parameters.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(function_mutual_recursion_test) { expect_script_file_out_to_be("function/mutual_recursion.lox", "true\ntrue\n"); }
BOOST_AUTO_TEST_CASE(function_parameters_test) { expect_script_file_out_to_be("function/parameters.lox", "0\n1\n3\n6\n10\n15\n21\n28\n36\n"); }
BOOST_AUTO_TEST_CASE(function_print_test) { expect_script_file_out_to_be("function/print.lox", "<fn foo>\n"); }
BOOST_AUTO_TEST_CASE(function_recursion_test) { expect_script_file_out_to_be("function/recursion.lox", "21\n"); }
BOOST_AUTO_TEST_CASE(function_too_many_arguments_test) { expect_script_file_out_to_be("function/too_many_arguments.lox", "", "[Line 1] Error at ')': Cannot have more than 8 arguments.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(function_too_many_parameters_test) { expect_script_file_out_to_be("function/too_many_parameters.lox", "", "[Line 2] Error at ')': Cannot have more than 8 parameters.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(if_dangling_else_test) { expect_script_file_out_to_be("if/dangling_else.lox", "good\n"); }
BOOST_AUTO_TEST_CASE(if_else_test) { expect_script_file_out_to_be("if/else.lox", "good\ngood\nblock\n"); }
BOOST_AUTO_TEST_CASE(if_fun_in_else_test) { expect_script_file_out_to_be("if/fun_in_else.lox", "", "[Line 2] Error at 'fun': Expected expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(if_fun_in_then_test) { expect_script_file_out_to_be("if/fun_in_then.lox", "", "[Line 2] Error at 'fun': Expected expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(if_if_test) { expect_script_file_out_to_be("if/if.lox", "good\nblock\ntrue\n"); }
BOOST_AUTO_TEST_CASE(if_truth_test) { expect_script_file_out_to_be("if/truth.lox", "false\nnil\ntrue\n0\nempty\n"); }
BOOST_AUTO_TEST_CASE(if_var_in_else_test) { expect_script_file_out_to_be("if/var_in_else.lox", "", "[Line 2] Error at 'var': Expected expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(if_var_in_then_test) { expect_script_file_out_to_be("if/var_in_then.lox", "", "[Line 2] Error at 'var': Expected expression.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(logical_operator_and_test) { expect_script_file_out_to_be("logical_operator/and.lox", "false\n1\nfalse\ntrue\n3\ntrue\nfalse\n"); }
BOOST_AUTO_TEST_CASE(logical_operator_and_truth_test) { expect_script_file_out_to_be("logical_operator/and_truth.lox", "false\nnil\nok\nok\nok\n"); }
BOOST_AUTO_TEST_CASE(logical_operator_or_test) { expect_script_file_out_to_be("logical_operator/or.lox", "1\n1\ntrue\nfalse\nfalse\nfalse\ntrue\n"); }
BOOST_AUTO_TEST_CASE(logical_operator_or_truth_test) { expect_script_file_out_to_be("logical_operator/or_truth.lox", "ok\nok\ntrue\n0\ns\n"); }

BOOST_AUTO_TEST_CASE(nil_literal_test) { expect_script_file_out_to_be("nil/literal.lox", "nil\n"); }

BOOST_AUTO_TEST_CASE(number_decimal_point_at_eof_test) { expect_script_file_out_to_be("number/decimal_point_at_eof.lox", "", "[Line 2] Error at '.': Expected ';' after expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(number_leading_dot_test) { expect_script_file_out_to_be("number/leading_dot.lox", "", "[Line 2] Error at '.': Expected expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(number_literals_test) { expect_script_file_out_to_be("number/literals.lox", "123\n987654\n0\n-0\n123.456\n-0.001\n"); }
BOOST_AUTO_TEST_CASE(number_trailing_dot_test) { expect_script_file_out_to_be("number/trailing_dot.lox", "", "[Line 2] Error at '.': Expected ';' after expression.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(operator_add_test) { expect_script_file_out_to_be("operator/add.lox", "579\nstring\n"); }
BOOST_AUTO_TEST_CASE(operator_add_bool_nil_test) { expect_script_file_out_to_be("operator/add_bool_nil.lox", "", "Operands must be two numbers or two strings.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_add_bool_num_test) { expect_script_file_out_to_be("operator/add_bool_num.lox", "", "Operands must be two numbers or two strings.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_add_bool_string_test) { expect_script_file_out_to_be("operator/add_bool_string.lox", "", "Operands must be two numbers or two strings.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_add_nil_nil_test) { expect_script_file_out_to_be("operator/add_nil_nil.lox", "", "Operands must be two numbers or two strings.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_add_num_nil_test) { expect_script_file_out_to_be("operator/add_num_nil.lox", "", "Operands must be two numbers or two strings.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_add_string_nil_test) { expect_script_file_out_to_be("operator/add_string_nil.lox", "", "Operands must be two numbers or two strings.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_comparison_test) { expect_script_file_out_to_be("operator/comparison.lox", "true\nfalse\nfalse\ntrue\ntrue\nfalse\nfalse\nfalse\ntrue\nfalse\ntrue\ntrue\nfalse\nfalse\nfalse\nfalse\ntrue\ntrue\ntrue\ntrue\n"); }
BOOST_AUTO_TEST_CASE(operator_divide_test) { expect_script_file_out_to_be("operator/divide.lox", "4\n1\n"); }
BOOST_AUTO_TEST_CASE(operator_divide_nonnum_num_test) { expect_script_file_out_to_be("operator/divide_nonnum_num.lox", "", "[Line 1] Error '/': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_divide_num_nonnum_test) { expect_script_file_out_to_be("operator/divide_num_nonnum.lox", "", "[Line 1] Error '/': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_equals_test) { expect_script_file_out_to_be("operator/equals.lox", "true\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\nfalse\nfalse\nfalse\n"); }
BOOST_AUTO_TEST_CASE(operator_greater_nonnum_num_test) { expect_script_file_out_to_be("operator/greater_nonnum_num.lox", "", "[Line 1] Error '>': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_greater_num_nonnum_test) { expect_script_file_out_to_be("operator/greater_num_nonnum.lox", "", "[Line 1] Error '>': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_greater_or_equal_nonnum_num_test) { expect_script_file_out_to_be("operator/greater_or_equal_nonnum_num.lox", "", "[Line 1] Error '>=': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_greater_or_equal_num_nonnum_test) { expect_script_file_out_to_be("operator/greater_or_equal_num_nonnum.lox", "", "[Line 1] Error '>=': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_less_nonnum_num_test) { expect_script_file_out_to_be("operator/less_nonnum_num.lox", "", "[Line 1] Error '<': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_less_num_nonnum_test) { expect_script_file_out_to_be("operator/less_num_nonnum.lox", "", "[Line 1] Error '<': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_less_or_equal_nonnum_num_test) { expect_script_file_out_to_be("operator/less_or_equal_nonnum_num.lox", "", "[Line 1] Error '<=': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_less_or_equal_num_nonnum_test) { expect_script_file_out_to_be("operator/less_or_equal_num_nonnum.lox", "", "[Line 1] Error '<=': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_multiply_test) { expect_script_file_out_to_be("operator/multiply.lox", "15\n3.702\n"); }
BOOST_AUTO_TEST_CASE(operator_multiply_nonnum_num_test) { expect_script_file_out_to_be("operator/multiply_nonnum_num.lox", "", "[Line 1] Error '*': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_multiply_num_nonnum_test) { expect_script_file_out_to_be("operator/multiply_num_nonnum.lox", "", "[Line 1] Error '*': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_negate_test) { expect_script_file_out_to_be("operator/negate.lox", "-3\n3\n-3\n"); }
BOOST_AUTO_TEST_CASE(operator_negate_nonnum_test) { expect_script_file_out_to_be("operator/negate_nonnum.lox", "", "[Line 1] Error '-': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_not_test) { expect_script_file_out_to_be("operator/not.lox", "false\ntrue\ntrue\nfalse\nfalse\ntrue\nfalse\nfalse\n"); }
BOOST_AUTO_TEST_CASE(operator_not_equals_test) { expect_script_file_out_to_be("operator/not_equals.lox", "false\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\ntrue\ntrue\ntrue\n"); }
BOOST_AUTO_TEST_CASE(operator_subtract_test) { expect_script_file_out_to_be("operator/subtract.lox", "1\n0\n"); }
BOOST_AUTO_TEST_CASE(operator_subtract_nonnum_num_test) { expect_script_file_out_to_be("operator/subtract_nonnum_num.lox", "", "[Line 1] Error '-': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_subtract_num_nonnum_test) { expect_script_file_out_to_be("operator/subtract_num_nonnum.lox", "", "[Line 1] Error '-': Operands must be numbers.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(print_missing_argument_test) { expect_script_file_out_to_be("print/missing_argument.lox", "", "[Line 2] Error at ';': Expected expression.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(regression_40_test) { expect_script_file_out_to_be("regression/40.lox", "false\n"); }

BOOST_AUTO_TEST_CASE(return_after_else_test) { expect_script_file_out_to_be("return/after_else.lox", "ok\n"); }
BOOST_AUTO_TEST_CASE(return_after_if_test) { expect_script_file_out_to_be("return/after_if.lox", "ok\n"); }
BOOST_AUTO_TEST_CASE(return_after_while_test) { expect_script_file_out_to_be("return/after_while.lox", "ok\n"); }
BOOST_AUTO_TEST_CASE(return_in_function_test) { expect_script_file_out_to_be("return/in_function.lox", "ok\n"); }
BOOST_AUTO_TEST_CASE(return_return_nil_if_no_value_test) { expect_script_file_out_to_be("return/return_nil_if_no_value.lox", "nil\n"); }

BOOST_AUTO_TEST_CASE(string_error_after_multiline_test) { expect_script_file_out_to_be("string/error_after_multiline.lox", "", "Undefined variable 'err'.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(string_literals_test) { expect_script_file_out_to_be("string/literals.lox", "()\na string\nA~¶Þॐஃ\n"); }
BOOST_AUTO_TEST_CASE(string_multiline_test) { expect_script_file_out_to_be("string/multiline.lox", "1\n2\n3\n"); }
BOOST_AUTO_TEST_CASE(string_unterminated_test) { expect_script_file_out_to_be("string/unterminated.lox", "", "[Line 2] Error: Unterminated string.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(variable_in_middle_of_block_test) { expect_script_file_out_to_be("variable/in_middle_of_block.lox", "a\na b\na c\na b d\n"); }
BOOST_AUTO_TEST_CASE(variable_in_nested_block_test) { expect_script_file_out_to_be("variable/in_nested_block.lox", "outer\n"); }
BOOST_AUTO_TEST_CASE(variable_redeclare_global_test) { expect_script_file_out_to_be("variable/redeclare_global.lox", "nil\n"); }
BOOST_AUTO_TEST_CASE(variable_redefine_global_test) { expect_script_file_out_to_be("variable/redefine_global.lox", "2\n"); }
BOOST_AUTO_TEST_CASE(variable_scope_reuse_in_different_blocks_test) { expect_script_file_out_to_be("variable/scope_reuse_in_different_blocks.lox", "first\nsecond\n"); }
BOOST_AUTO_TEST_CASE(variable_shadow_and_local_test) { expect_script_file_out_to_be("variable/shadow_and_local.lox", "outer\ninner\n"); }
BOOST_AUTO_TEST_CASE(variable_shadow_global_test) { expect_script_file_out_to_be("variable/shadow_global.lox", "shadow\nglobal\n"); }
BOOST_AUTO_TEST_CASE(variable_shadow_local_test) { expect_script_file_out_to_be("variable/shadow_local.lox", "shadow\nlocal\n"); }
BOOST_AUTO_TEST_CASE(variable_undefined_global_test) { expect_script_file_out_to_be("variable/undefined_global.lox", "", "Undefined variable 'notDefined'.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(variable_undefined_local_test) { expect_script_file_out_to_be("variable/undefined_local.lox", "", "Undefined variable 'notDefined'.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(variable_uninitialized_test) { expect_script_file_out_to_be("variable/uninitialized.lox", "nil\n"); }
BOOST_AUTO_TEST_CASE(variable_unreached_undefined_test) { expect_script_file_out_to_be("variable/unreached_undefined.lox", "ok\n"); }
BOOST_AUTO_TEST_CASE(variable_use_false_as_var_test) { expect_script_file_out_to_be("variable/use_false_as_var.lox", "", "[Line 2] Error at 'false': Expected variable name.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(variable_use_global_in_initializer_test) { expect_script_file_out_to_be("variable/use_global_in_initializer.lox", "value\n"); }
BOOST_AUTO_TEST_CASE(variable_use_nil_as_var_test) { expect_script_file_out_to_be("variable/use_nil_as_var.lox", "", "[Line 2] Error at 'nil': Expected variable name.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(while_closure_in_body_test) { expect_script_file_out_to_be("while/closure_in_body.lox", "1\n2\n3\n"); }
BOOST_AUTO_TEST_CASE(while_fun_in_body_test) { expect_script_file_out_to_be("while/fun_in_body.lox", "", "[Line 2] Error at 'fun': Expected expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(while_return_closure_test) { expect_script_file_out_to_be("while/return_closure.lox", "i\n"); }
BOOST_AUTO_TEST_CASE(while_return_inside_test) { expect_script_file_out_to_be("while/return_inside.lox", "i\n"); }
BOOST_AUTO_TEST_CASE(while_syntax_test) { expect_script_file_out_to_be("while/syntax.lox", "1\n2\n3\n0\n1\n2\n"); }
BOOST_AUTO_TEST_CASE(while_var_in_body_test) { expect_script_file_out_to_be("while/var_in_body.lox", "", "[Line 2] Error at 'var': Expected expression.\n", EXIT_FAILURE); }
