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

BOOST_AUTO_TEST_CASE(empty_file_test) { expect_script_file_out_to_be("empty_file.lox", ""); }
BOOST_AUTO_TEST_CASE(precedence_test) { expect_script_file_out_to_be("precedence.lox", "14\n8\n4\n0\ntrue\ntrue\ntrue\ntrue\n0\n0\n0\n0\n4\n"); }
BOOST_AUTO_TEST_CASE(unexpected_character_test) { expect_script_file_out_to_be("unexpected_character.lox", "", "[Line 3] Error: Unexpected character.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(assignment_associativity_test) { expect_script_file_out_to_be("assignment/associativity.lox", "c\nc\nc\n"); }
BOOST_AUTO_TEST_CASE(assignment_global_test) { expect_script_file_out_to_be("assignment/global.lox", "before\nafter\narg\narg\n"); }
BOOST_AUTO_TEST_CASE(assignment_grouping_test) { expect_script_file_out_to_be("assignment/grouping.lox", "", "[Line 2] Error at '=': Invalid assignment target.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(assignment_infix_operator_test) { expect_script_file_out_to_be("assignment/infix_operator.lox", "", "[Line 3] Error at '=': Invalid assignment target.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(assignment_local_test) { expect_script_file_out_to_be("assignment/local.lox", "before\nafter\narg\narg\n"); }
BOOST_AUTO_TEST_CASE(assignment_prefix_operator_test) { expect_script_file_out_to_be("assignment/prefix_operator.lox", "", "[Line 2] Error at '=': Invalid assignment target.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(assignment_syntax_test) { expect_script_file_out_to_be("assignment/syntax.lox", "var\nvar\n"); }
BOOST_AUTO_TEST_CASE(assignment_to_this_test) { expect_script_file_out_to_be("assignment/to_this.lox", "", "[Line 3] Error at '=': Invalid assignment target.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(assignment_undefined_test) { expect_script_file_out_to_be("assignment/undefined.lox", "", "Undefined variable 'unknown'.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(block_empty_test) { expect_script_file_out_to_be("block/empty.lox", "ok\n"); }
BOOST_AUTO_TEST_CASE(block_scope_test) { expect_script_file_out_to_be("block/scope.lox", "inner\nouter\n"); }

BOOST_AUTO_TEST_CASE(bool_equality_test) { expect_script_file_out_to_be("bool/equality.lox", "true\nfalse\nfalse\ntrue\n" "false\nfalse\nfalse\nfalse\nfalse\n" "false\ntrue\ntrue\nfalse\n" "true\ntrue\ntrue\ntrue\ntrue\n"); }
BOOST_AUTO_TEST_CASE(bool_not_test) { expect_script_file_out_to_be("bool/not.lox", "false\ntrue\ntrue\n"); }

BOOST_AUTO_TEST_CASE(call_bool_test) { expect_script_file_out_to_be("call/bool.lox", "", "Can only call functions and classes.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(call_nil_test) { expect_script_file_out_to_be("call/nil.lox", "", "Can only call functions and classes.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(call_num_test) { expect_script_file_out_to_be("call/num.lox", "", "Can only call functions and classes.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(call_object_test) { expect_script_file_out_to_be("call/object.lox", "", "Can only call functions and classes.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(call_string_test) { expect_script_file_out_to_be("call/string.lox", "", "Can only call functions and classes.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(class_empty_test) { expect_script_file_out_to_be("class/empty.lox", "Foo\n"); }
BOOST_AUTO_TEST_CASE(class_inherited_method_test) { expect_script_file_out_to_be("class/inherited_method.lox", "in foo\nin bar\nin baz\n"); }
BOOST_AUTO_TEST_CASE(class_local_reference_self_test) { expect_script_file_out_to_be("class/local_reference_self.lox", "Foo\n"); }
BOOST_AUTO_TEST_CASE(class_reference_self_test) { expect_script_file_out_to_be("class/reference_self.lox", "Foo\n"); }

BOOST_AUTO_TEST_CASE(closure_assign_to_closure_test) { expect_script_file_out_to_be("closure/assign_to_closure.lox", "local\nafter f\nafter f\nafter g\n"); }
BOOST_AUTO_TEST_CASE(closure_assign_to_shadowed_later_test) { expect_script_file_out_to_be("closure/assign_to_shadowed_later.lox", "inner\nassigned\n"); }
BOOST_AUTO_TEST_CASE(closure_close_over_function_parameter_test) { expect_script_file_out_to_be("closure/close_over_function_parameter.lox", "param\n"); }
BOOST_AUTO_TEST_CASE(closure_close_over_method_parameter_test) { expect_script_file_out_to_be("closure/close_over_method_parameter.lox", "param\n"); }
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

BOOST_AUTO_TEST_CASE(constructor_arguments_test) { expect_script_file_out_to_be("constructor/arguments.lox", "init\n1\n2\n"); }
BOOST_AUTO_TEST_CASE(constructor_call_init_explicitly_test) { expect_script_file_out_to_be("constructor/call_init_explicitly.lox", "Foo.init(one)\nFoo.init(two)\nFoo instance\ninit\n"); }
BOOST_AUTO_TEST_CASE(constructor_default_test) { expect_script_file_out_to_be("constructor/default.lox", "Foo instance\n"); }
BOOST_AUTO_TEST_CASE(constructor_default_arguments_test) { expect_script_file_out_to_be("constructor/default_arguments.lox", "", "[Line 3] Error at ')': Expected 0 arguments but got 3.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(constructor_early_return_test) { expect_script_file_out_to_be("constructor/early_return.lox", "init\nFoo instance\n"); }
BOOST_AUTO_TEST_CASE(constructor_extra_arguments_test) { expect_script_file_out_to_be("constructor/extra_arguments.lox", "", "[Line 8] Error at ')': Expected 2 arguments but got 4.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(constructor_init_not_method_test) { expect_script_file_out_to_be("constructor/init_not_method.lox", "not initializer\n"); }
BOOST_AUTO_TEST_CASE(constructor_missing_arguments_test) { expect_script_file_out_to_be("constructor/missing_arguments.lox", "", "[Line 5] Error at ')': Expected 2 arguments but got 1.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(constructor_return_in_nested_function_test) { expect_script_file_out_to_be("constructor/return_in_nested_function.lox", "bar\nFoo instance\n"); }
BOOST_AUTO_TEST_CASE(constructor_return_value_test) { expect_script_file_out_to_be("constructor/return_value.lox", "", "[Line 3] Error at 'return': Cannot return a value from an initializer.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(field_call_function_field_test) { expect_script_file_out_to_be("field/call_function_field.lox", "bar\n"); }
BOOST_AUTO_TEST_CASE(field_call_nonfunction_field_test) { expect_script_file_out_to_be("field/call_nonfunction_field.lox", "", "Can only call functions and classes.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(field_get_and_set_method_test) { expect_script_file_out_to_be("field/get_and_set_method.lox", "other\nmethod\n"); }
BOOST_AUTO_TEST_CASE(field_get_on_bool_test) { expect_script_file_out_to_be("field/get_on_bool.lox", "", "[Line 1] Error at 'foo': Only instances have properties.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(field_get_on_class_test) { expect_script_file_out_to_be("field/get_on_class.lox", "", "[Line 2] Error at 'bar': Only instances have properties.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(field_get_on_function_test) { expect_script_file_out_to_be("field/get_on_function.lox", "", "[Line 3] Error at 'bar': Only instances have properties.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(field_get_on_nil_test) { expect_script_file_out_to_be("field/get_on_nil.lox", "", "[Line 1] Error at 'foo': Only instances have properties.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(field_get_on_num_test) { expect_script_file_out_to_be("field/get_on_num.lox", "", "[Line 1] Error at 'foo': Only instances have properties.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(field_get_on_string_test) { expect_script_file_out_to_be("field/get_on_string.lox", "", "[Line 1] Error at 'foo': Only instances have properties.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(field_many_test) { expect_script_file_out_to_be(
    "field/many.lox",
    "apple\n"
    "apricot\n"
    "avocado\n"
    "banana\n"
    "bilberry\n"
    "blackberry\n"
    "blackcurrant\n"
    "blueberry\n"
    "boysenberry\n"
    "cantaloupe\n"
    "cherimoya\n"
    "cherry\n"
    "clementine\n"
    "cloudberry\n"
    "coconut\n"
    "cranberry\n"
    "currant\n"
    "damson\n"
    "date\n"
    "dragonfruit\n"
    "durian\n"
    "elderberry\n"
    "feijoa\n"
    "fig\n"
    "gooseberry\n"
    "grape\n"
    "grapefruit\n"
    "guava\n"
    "honeydew\n"
    "huckleberry\n"
    "jabuticaba\n"
    "jackfruit\n"
    "jambul\n"
    "jujube\n"
    "juniper\n"
    "kiwifruit\n"
    "kumquat\n"
    "lemon\n"
    "lime\n"
    "longan\n"
    "loquat\n"
    "lychee\n"
    "mandarine\n"
    "mango\n"
    "marionberry\n"
    "melon\n"
    "miracle\n"
    "mulberry\n"
    "nance\n"
    "nectarine\n"
    "olive\n"
    "orange\n"
    "papaya\n"
    "passionfruit\n"
    "peach\n"
    "pear\n"
    "persimmon\n"
    "physalis\n"
    "pineapple\n"
    "plantain\n"
    "plum\n"
    "plumcot\n"
    "pomegranate\n"
    "pomelo\n"
    "quince\n"
    "raisin\n"
    "rambutan\n"
    "raspberry\n"
    "redcurrant\n"
    "salak\n"
    "salmonberry\n"
    "satsuma\n"
    "strawberry\n"
    "tamarillo\n"
    "tamarind\n"
    "tangerine\n"
    "tomato\n"
    "watermelon\n"
    "yuzu\n"
); }
BOOST_AUTO_TEST_CASE(field_method_test) { expect_script_file_out_to_be("field/method.lox", "got method\narg\n"); }
BOOST_AUTO_TEST_CASE(field_method_binds_this_test) { expect_script_file_out_to_be("field/method_binds_this.lox", "foo1\n"); }
BOOST_AUTO_TEST_CASE(field_on_instance_test) { expect_script_file_out_to_be("field/on_instance.lox", "bar value\nbaz value\nbar value\nbaz value\n"); }
BOOST_AUTO_TEST_CASE(field_set_evaluation_order_test) { expect_script_file_out_to_be("field/set_evaluation_order.lox", "", "Undefined variable 'undefined1'.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(field_set_on_bool_test) { expect_script_file_out_to_be("field/set_on_bool.lox", "", "[Line 1] Error at 'foo': Only instances have fields.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(field_set_on_class_test) { expect_script_file_out_to_be("field/set_on_class.lox", "", "[Line 2] Error at 'bar': Only instances have fields.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(field_set_on_function_test) { expect_script_file_out_to_be("field/set_on_function.lox", "", "[Line 3] Error at 'bar': Only instances have fields.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(field_set_on_nil_test) { expect_script_file_out_to_be("field/set_on_nil.lox", "", "[Line 1] Error at 'foo': Only instances have fields.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(field_set_on_num_test) { expect_script_file_out_to_be("field/set_on_num.lox", "", "[Line 1] Error at 'foo': Only instances have fields.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(field_set_on_string_test) { expect_script_file_out_to_be("field/set_on_string.lox", "", "[Line 1] Error at 'foo': Only instances have fields.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(field_undefined_test) { expect_script_file_out_to_be("field/undefined.lox", "", "Undefined property 'bar'.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(for_class_in_body_test) { expect_script_file_out_to_be("for/class_in_body.lox", "", "[Line 2] Error at 'class': Expected expression.\n", EXIT_FAILURE); }
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
BOOST_AUTO_TEST_CASE(function_extra_arguments_test) { expect_script_file_out_to_be("function/extra_arguments.lox", "", "[Line 6] Error at ')': Expected 2 arguments but got 4.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(function_local_mutual_recursion_test) { expect_script_file_out_to_be("function/local_mutual_recursion.lox", "", "Undefined variable 'isOdd'.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(function_local_recursion_test) { expect_script_file_out_to_be("function/local_recursion.lox", "21\n"); }
BOOST_AUTO_TEST_CASE(function_missing_arguments_test) { expect_script_file_out_to_be("function/missing_arguments.lox", "", "[Line 3] Error at ')': Expected 2 arguments but got 1.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(function_missing_comma_in_parameters_test) { expect_script_file_out_to_be("function/missing_comma_in_parameters.lox", "", "[Line 3] Error at 'c': Expected ')' after parameters.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(function_mutual_recursion_test) { expect_script_file_out_to_be("function/mutual_recursion.lox", "true\ntrue\n"); }
BOOST_AUTO_TEST_CASE(function_parameters_test) { expect_script_file_out_to_be("function/parameters.lox", "0\n1\n3\n6\n10\n15\n21\n28\n36\n"); }
BOOST_AUTO_TEST_CASE(function_print_test) { expect_script_file_out_to_be("function/print.lox", "<fn foo>\n"); }
BOOST_AUTO_TEST_CASE(function_recursion_test) { expect_script_file_out_to_be("function/recursion.lox", "21\n"); }
BOOST_AUTO_TEST_CASE(function_too_many_arguments_test) { expect_script_file_out_to_be("function/too_many_arguments.lox", "", "[Line 1] Error at ')': Cannot have more than 8 arguments.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(function_too_many_parameters_test) { expect_script_file_out_to_be("function/too_many_parameters.lox", "", "[Line 2] Error at ')': Cannot have more than 8 parameters.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(if_class_in_else_test) { expect_script_file_out_to_be("if/class_in_else.lox", "", "[Line 2] Error at 'class': Expected expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(if_class_in_then_test) { expect_script_file_out_to_be("if/class_in_then.lox", "", "[Line 2] Error at 'class': Expected expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(if_dangling_else_test) { expect_script_file_out_to_be("if/dangling_else.lox", "good\n"); }
BOOST_AUTO_TEST_CASE(if_else_test) { expect_script_file_out_to_be("if/else.lox", "good\ngood\nblock\n"); }
BOOST_AUTO_TEST_CASE(if_fun_in_else_test) { expect_script_file_out_to_be("if/fun_in_else.lox", "", "[Line 2] Error at 'fun': Expected expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(if_fun_in_then_test) { expect_script_file_out_to_be("if/fun_in_then.lox", "", "[Line 2] Error at 'fun': Expected expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(if_if_test) { expect_script_file_out_to_be("if/if.lox", "good\nblock\ntrue\n"); }
BOOST_AUTO_TEST_CASE(if_truth_test) { expect_script_file_out_to_be("if/truth.lox", "false\nnil\ntrue\n0\nempty\n"); }
BOOST_AUTO_TEST_CASE(if_var_in_else_test) { expect_script_file_out_to_be("if/var_in_else.lox", "", "[Line 2] Error at 'var': Expected expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(if_var_in_then_test) { expect_script_file_out_to_be("if/var_in_then.lox", "", "[Line 2] Error at 'var': Expected expression.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(inheritance_inherit_from_function_test) { expect_script_file_out_to_be("inheritance/inherit_from_function.lox", "", "[Line 3] Error at 'foo': Superclass must be a class.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(inheritance_inherit_from_nil_test) { expect_script_file_out_to_be("inheritance/inherit_from_nil.lox", "", "[Line 2] Error at 'Nil': Superclass must be a class.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(inheritance_inherit_from_number_test) { expect_script_file_out_to_be("inheritance/inherit_from_number.lox", "", "[Line 2] Error at 'Number': Superclass must be a class.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(inheritance_inherit_methods_test) { expect_script_file_out_to_be("inheritance/inherit_methods.lox", "foo\nbar\nbar\n"); }
BOOST_AUTO_TEST_CASE(inheritance_parenthesized_superclass_test) { expect_script_file_out_to_be("inheritance/parenthesized_superclass.lox", "", "[Line 4] Error at '(': Expected superclass name.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(inheritance_set_fields_from_base_class_test) { expect_script_file_out_to_be("inheritance/set_fields_from_base_class.lox", "foo 1\nfoo 2\nbar 1\nbar 2\nbar 1\nbar 2\n"); }

BOOST_AUTO_TEST_CASE(logical_operator_and_test) { expect_script_file_out_to_be("logical_operator/and.lox", "false\n1\nfalse\ntrue\n3\ntrue\nfalse\n"); }
BOOST_AUTO_TEST_CASE(logical_operator_and_truth_test) { expect_script_file_out_to_be("logical_operator/and_truth.lox", "false\nnil\nok\nok\nok\n"); }
BOOST_AUTO_TEST_CASE(logical_operator_or_test) { expect_script_file_out_to_be("logical_operator/or.lox", "1\n1\ntrue\nfalse\nfalse\nfalse\ntrue\n"); }
BOOST_AUTO_TEST_CASE(logical_operator_or_truth_test) { expect_script_file_out_to_be("logical_operator/or_truth.lox", "ok\nok\ntrue\n0\ns\n"); }

BOOST_AUTO_TEST_CASE(method_arity_test) { expect_script_file_out_to_be("method/arity.lox", "no args\n1\n3\n6\n10\n15\n21\n28\n36\n"); }
BOOST_AUTO_TEST_CASE(method_empty_block_test) { expect_script_file_out_to_be("method/empty_block.lox", "nil\n"); }
BOOST_AUTO_TEST_CASE(method_extra_arguments_test) { expect_script_file_out_to_be("method/extra_arguments.lox", "", "[Line 8] Error at ')': Expected 2 arguments but got 4.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(method_missing_arguments_test) { expect_script_file_out_to_be("method/missing_arguments.lox", "", "[Line 5] Error at ')': Expected 2 arguments but got 1.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(method_not_found_test) { expect_script_file_out_to_be("method/not_found.lox", "", "Undefined property 'unknown'.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(method_refer_to_name_test) { expect_script_file_out_to_be("method/refer_to_name.lox", "", "Undefined variable 'method'.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(method_too_many_arguments_test) { expect_script_file_out_to_be("method/too_many_arguments.lox", "", "[Line 1] Error at ')': Cannot have more than 8 arguments.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(method_too_many_parameters_test) { expect_script_file_out_to_be("method/too_many_parameters.lox", "", "[Line 3] Error at ')': Cannot have more than 8 parameters.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(nil_literal_test) { expect_script_file_out_to_be("nil/literal.lox", "nil\n"); }

BOOST_AUTO_TEST_CASE(number_decimal_point_at_eof_test) { expect_script_file_out_to_be("number/decimal_point_at_eof.lox", "", "[Line 2] Error at end: Expected property name after '.'.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(number_leading_dot_test) { expect_script_file_out_to_be("number/leading_dot.lox", "", "[Line 2] Error at '.': Expected expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(number_literals_test) { expect_script_file_out_to_be("number/literals.lox", "123\n987654\n0\n-0\n123.456\n-0.001\n"); }
BOOST_AUTO_TEST_CASE(number_trailing_dot_test) { expect_script_file_out_to_be("number/trailing_dot.lox", "", "[Line 2] Error at ';': Expected property name after '.'.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(operator_add_test) { expect_script_file_out_to_be("operator/add.lox", "579\nstring\n"); }
BOOST_AUTO_TEST_CASE(operator_add_bool_nil_test) { expect_script_file_out_to_be("operator/add_bool_nil.lox", "", "Operands must be two numbers or two strings.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_add_bool_num_test) { expect_script_file_out_to_be("operator/add_bool_num.lox", "", "Operands must be two numbers or two strings.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_add_bool_string_test) { expect_script_file_out_to_be("operator/add_bool_string.lox", "", "Operands must be two numbers or two strings.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_add_nil_nil_test) { expect_script_file_out_to_be("operator/add_nil_nil.lox", "", "Operands must be two numbers or two strings.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_add_num_nil_test) { expect_script_file_out_to_be("operator/add_num_nil.lox", "", "Operands must be two numbers or two strings.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_add_string_nil_test) { expect_script_file_out_to_be("operator/add_string_nil.lox", "", "Operands must be two numbers or two strings.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_comparison_test) { expect_script_file_out_to_be("operator/comparison.lox", "true\nfalse\nfalse\ntrue\ntrue\nfalse\nfalse\nfalse\ntrue\nfalse\ntrue\ntrue\nfalse\nfalse\nfalse\nfalse\ntrue\ntrue\ntrue\ntrue\n"); }
BOOST_AUTO_TEST_CASE(operator_divide_test) { expect_script_file_out_to_be("operator/divide.lox", "4\n1\n"); }
BOOST_AUTO_TEST_CASE(operator_divide_nonnum_num_test) { expect_script_file_out_to_be("operator/divide_nonnum_num.lox", "", "[Line 1] Error at '/': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_divide_num_nonnum_test) { expect_script_file_out_to_be("operator/divide_num_nonnum.lox", "", "[Line 1] Error at '/': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_equals_test) { expect_script_file_out_to_be("operator/equals.lox", "true\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\nfalse\nfalse\nfalse\n"); }
BOOST_AUTO_TEST_CASE(operator_equals_class_test) { expect_script_file_out_to_be("operator/equals_class.lox", "true\nfalse\nfalse\ntrue\nfalse\nfalse\nfalse\nfalse\n"); }
BOOST_AUTO_TEST_CASE(operator_equals_method_test) { expect_script_file_out_to_be("operator/equals_method.lox", "true\nfalse\n"); }
BOOST_AUTO_TEST_CASE(operator_greater_nonnum_num_test) { expect_script_file_out_to_be("operator/greater_nonnum_num.lox", "", "[Line 1] Error at '>': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_greater_num_nonnum_test) { expect_script_file_out_to_be("operator/greater_num_nonnum.lox", "", "[Line 1] Error at '>': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_greater_or_equal_nonnum_num_test) { expect_script_file_out_to_be("operator/greater_or_equal_nonnum_num.lox", "", "[Line 1] Error at '>=': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_greater_or_equal_num_nonnum_test) { expect_script_file_out_to_be("operator/greater_or_equal_num_nonnum.lox", "", "[Line 1] Error at '>=': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_less_nonnum_num_test) { expect_script_file_out_to_be("operator/less_nonnum_num.lox", "", "[Line 1] Error at '<': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_less_num_nonnum_test) { expect_script_file_out_to_be("operator/less_num_nonnum.lox", "", "[Line 1] Error at '<': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_less_or_equal_nonnum_num_test) { expect_script_file_out_to_be("operator/less_or_equal_nonnum_num.lox", "", "[Line 1] Error at '<=': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_less_or_equal_num_nonnum_test) { expect_script_file_out_to_be("operator/less_or_equal_num_nonnum.lox", "", "[Line 1] Error at '<=': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_multiply_test) { expect_script_file_out_to_be("operator/multiply.lox", "15\n3.702\n"); }
BOOST_AUTO_TEST_CASE(operator_multiply_nonnum_num_test) { expect_script_file_out_to_be("operator/multiply_nonnum_num.lox", "", "[Line 1] Error at '*': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_multiply_num_nonnum_test) { expect_script_file_out_to_be("operator/multiply_num_nonnum.lox", "", "[Line 1] Error at '*': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_negate_test) { expect_script_file_out_to_be("operator/negate.lox", "-3\n3\n-3\n"); }
BOOST_AUTO_TEST_CASE(operator_negate_nonnum_test) { expect_script_file_out_to_be("operator/negate_nonnum.lox", "", "[Line 1] Error at '-': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_not_test) { expect_script_file_out_to_be("operator/not.lox", "false\ntrue\ntrue\nfalse\nfalse\ntrue\nfalse\nfalse\n"); }
BOOST_AUTO_TEST_CASE(operator_not_class_test) { expect_script_file_out_to_be("operator/not_class.lox", "false\nfalse\n"); }
BOOST_AUTO_TEST_CASE(operator_not_equals_test) { expect_script_file_out_to_be("operator/not_equals.lox", "false\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\ntrue\ntrue\ntrue\n"); }
BOOST_AUTO_TEST_CASE(operator_subtract_test) { expect_script_file_out_to_be("operator/subtract.lox", "1\n0\n"); }
BOOST_AUTO_TEST_CASE(operator_subtract_nonnum_num_test) { expect_script_file_out_to_be("operator/subtract_nonnum_num.lox", "", "[Line 1] Error at '-': Operands must be numbers.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(operator_subtract_num_nonnum_test) { expect_script_file_out_to_be("operator/subtract_num_nonnum.lox", "", "[Line 1] Error at '-': Operands must be numbers.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(print_missing_argument_test) { expect_script_file_out_to_be("print/missing_argument.lox", "", "[Line 2] Error at ';': Expected expression.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(regression_40_test) { expect_script_file_out_to_be("regression/40.lox", "false\n"); }

BOOST_AUTO_TEST_CASE(return_after_else_test) { expect_script_file_out_to_be("return/after_else.lox", "ok\n"); }
BOOST_AUTO_TEST_CASE(return_after_if_test) { expect_script_file_out_to_be("return/after_if.lox", "ok\n"); }
BOOST_AUTO_TEST_CASE(return_after_while_test) { expect_script_file_out_to_be("return/after_while.lox", "ok\n"); }
BOOST_AUTO_TEST_CASE(return_at_top_level_test) { expect_script_file_out_to_be("return/at_top_level.lox", "", "[Line 1] Error at 'return': Cannot return from top-level code.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(return_in_function_test) { expect_script_file_out_to_be("return/in_function.lox", "ok\n"); }
BOOST_AUTO_TEST_CASE(return_in_method_test) { expect_script_file_out_to_be("return/in_method.lox", "ok\n"); }
BOOST_AUTO_TEST_CASE(return_return_nil_if_no_value_test) { expect_script_file_out_to_be("return/return_nil_if_no_value.lox", "nil\n"); }

BOOST_AUTO_TEST_CASE(string_error_after_multiline_test) { expect_script_file_out_to_be("string/error_after_multiline.lox", "", "Undefined variable 'err'.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(string_literals_test) { expect_script_file_out_to_be("string/literals.lox", "()\na string\nA~¶Þॐஃ\n"); }
BOOST_AUTO_TEST_CASE(string_multiline_test) { expect_script_file_out_to_be("string/multiline.lox", "1\n2\n3\n"); }
BOOST_AUTO_TEST_CASE(string_unterminated_test) { expect_script_file_out_to_be("string/unterminated.lox", "", "[Line 2] Error: Unterminated string.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(super_bound_method_test) { expect_script_file_out_to_be("super/bound_method.lox", "A.method(arg)\n"); }
BOOST_AUTO_TEST_CASE(super_call_other_method_test) { expect_script_file_out_to_be("super/call_other_method.lox", "Derived.bar()\nBase.foo()\n"); }
BOOST_AUTO_TEST_CASE(super_call_same_method_test) { expect_script_file_out_to_be("super/call_same_method.lox", "Derived.foo()\nBase.foo()\n"); }
BOOST_AUTO_TEST_CASE(super_closure_test) { expect_script_file_out_to_be("super/closure.lox", "Base\n"); }
BOOST_AUTO_TEST_CASE(super_constructor_test) { expect_script_file_out_to_be("super/constructor.lox", "Derived.init()\nBase.init(a, b)\n"); }
BOOST_AUTO_TEST_CASE(super_extra_arguments_test) { expect_script_file_out_to_be("super/extra_arguments.lox", "Derived.foo()\n", "[Line 10] Error at ')': Expected 2 arguments but got 4.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(super_indirectly_inherited_test) { expect_script_file_out_to_be("super/indirectly_inherited.lox", "C.foo()\nA.foo()\n"); }
BOOST_AUTO_TEST_CASE(super_missing_arguments_test) { expect_script_file_out_to_be("super/missing_arguments.lox", "", "[Line 9] Error at ')': Expected 2 arguments but got 1.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(super_no_superclass_bind_test) { expect_script_file_out_to_be("super/no_superclass_bind.lox", "", "[Line 3] Error at 'super': Cannot use 'super' in a class with no superclass.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(super_no_superclass_call_test) { expect_script_file_out_to_be("super/no_superclass_call.lox", "", "[Line 3] Error at 'super': Cannot use 'super' in a class with no superclass.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(super_no_superclass_method_test) { expect_script_file_out_to_be("super/no_superclass_method.lox", "", "Undefined property 'doesNotExist'.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(super_parenthesized_test) { expect_script_file_out_to_be("super/parenthesized.lox", "", "[Line 8] Error at ')': Expected '.' after 'super'.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(super_reassign_superclass_test) { expect_script_file_out_to_be("super/reassign_superclass.lox", "Base.method()\nBase.method()\n"); }
BOOST_AUTO_TEST_CASE(super_super_at_top_level_test) { expect_script_file_out_to_be("super/super_at_top_level.lox", "", "[Line 1] Error at 'super': Cannot use 'super' outside of a class.\n[Line 2] Error at 'super': Cannot use 'super' outside of a class.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(super_super_in_closure_in_inherited_method_test) { expect_script_file_out_to_be("super/super_in_closure_in_inherited_method.lox", "A\n"); }
BOOST_AUTO_TEST_CASE(super_super_in_inherited_method_test) { expect_script_file_out_to_be("super/super_in_inherited_method.lox", "A\n"); }
BOOST_AUTO_TEST_CASE(super_super_in_top_level_function_test) { expect_script_file_out_to_be("super/super_in_top_level_function.lox", "", "[Line 2] Error at 'super': Cannot use 'super' outside of a class.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(super_super_without_dot_test) { expect_script_file_out_to_be("super/super_without_dot.lox", "", "[Line 6] Error at ';': Expected '.' after 'super'.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(super_super_without_name_test) { expect_script_file_out_to_be("super/super_without_name.lox", "", "[Line 5] Error at ';': Expected superclass method name.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(super_this_in_superclass_method_test) { expect_script_file_out_to_be("super/this_in_superclass_method.lox", "a\nb\n"); }

BOOST_AUTO_TEST_CASE(this_closure_test) { expect_script_file_out_to_be("this/closure.lox", "Foo\n"); }
BOOST_AUTO_TEST_CASE(this_nested_class_test) { expect_script_file_out_to_be("this/nested_class.lox", "Outer instance\nOuter instance\nInner instance\n"); }
BOOST_AUTO_TEST_CASE(this_nested_closure_test) { expect_script_file_out_to_be("this/nested_closure.lox", "Foo\n"); }
BOOST_AUTO_TEST_CASE(this_this_at_top_level_test) { expect_script_file_out_to_be("this/this_at_top_level.lox", "", "[Line 1] Error at 'this': Cannot use 'this' outside of a class.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(this_this_in_method_test) { expect_script_file_out_to_be("this/this_in_method.lox", "baz\n"); }
BOOST_AUTO_TEST_CASE(this_this_in_top_level_function_test) { expect_script_file_out_to_be("this/this_in_top_level_function.lox", "", "[Line 2] Error at 'this': Cannot use 'this' outside of a class.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(variable_collide_with_parameter_test) { expect_script_file_out_to_be("variable/collide_with_parameter.lox", "", "[Line 2] Error at 'a': Variable with this name already declared in this scope.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(variable_duplicate_local_test) { expect_script_file_out_to_be("variable/duplicate_local.lox", "", "[Line 3] Error at 'a': Variable with this name already declared in this scope.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(variable_duplicate_parameter_test) { expect_script_file_out_to_be("variable/duplicate_parameter.lox", "", "[Line 2] Error at 'arg': Variable with this name already declared in this scope.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(variable_early_bound_test) { expect_script_file_out_to_be("variable/early_bound.lox", "outer\nouter\n"); }
BOOST_AUTO_TEST_CASE(variable_in_middle_of_block_test) { expect_script_file_out_to_be("variable/in_middle_of_block.lox", "a\na b\na c\na b d\n"); }
BOOST_AUTO_TEST_CASE(variable_in_nested_block_test) { expect_script_file_out_to_be("variable/in_nested_block.lox", "outer\n"); }
BOOST_AUTO_TEST_CASE(variable_local_from_method_test) { expect_script_file_out_to_be("variable/local_from_method.lox", "variable\n"); }
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
BOOST_AUTO_TEST_CASE(variable_use_local_in_initializer_test) { expect_script_file_out_to_be("variable/use_local_in_initializer.lox", "", "[Line 3] Error at 'a': Cannot read local variable in its own initializer.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(variable_use_nil_as_var_test) { expect_script_file_out_to_be("variable/use_nil_as_var.lox", "", "[Line 2] Error at 'nil': Expected variable name.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(variable_use_this_as_var_test) { expect_script_file_out_to_be("variable/use_this_as_var.lox", "", "[Line 2] Error at 'this': Expected variable name.\n", EXIT_FAILURE); }

BOOST_AUTO_TEST_CASE(while_class_in_body_test) { expect_script_file_out_to_be("while/class_in_body.lox", "", "[Line 2] Error at 'class': Expected expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(while_closure_in_body_test) { expect_script_file_out_to_be("while/closure_in_body.lox", "1\n2\n3\n"); }
BOOST_AUTO_TEST_CASE(while_fun_in_body_test) { expect_script_file_out_to_be("while/fun_in_body.lox", "", "[Line 2] Error at 'fun': Expected expression.\n", EXIT_FAILURE); }
BOOST_AUTO_TEST_CASE(while_return_closure_test) { expect_script_file_out_to_be("while/return_closure.lox", "i\n"); }
BOOST_AUTO_TEST_CASE(while_return_inside_test) { expect_script_file_out_to_be("while/return_inside.lox", "i\n"); }
BOOST_AUTO_TEST_CASE(while_syntax_test) { expect_script_file_out_to_be("while/syntax.lox", "1\n2\n3\n0\n1\n2\n"); }
BOOST_AUTO_TEST_CASE(while_var_in_body_test) { expect_script_file_out_to_be("while/var_in_body.lox", "", "[Line 2] Error at 'var': Expected expression.\n", EXIT_FAILURE); }
