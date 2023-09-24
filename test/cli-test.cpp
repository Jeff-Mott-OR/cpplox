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
        "cpploxbc ../src/test/lox/unexpected_character.lox",
        boost::process::std_out > cpplox_out,
        boost::process::std_err > cpplox_err
    );
    std::string actual_out {std::istreambuf_iterator<char>{cpplox_out}, {}};
    std::string actual_err {std::istreambuf_iterator<char>{cpplox_err}, {}};

    BOOST_TEST(actual_out == "");
    BOOST_TEST(actual_err == "[Line 3] Error: Unexpected character \"|\".\n");
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

// Create a series of functional tests that interact through the CLI, same as a use would do.
#define MOTTS_LOX_MAKE_TEST_CASE(TEST_NAME, TEST_FILE, EXPECTED_OUT, EXPECTED_ERR, EXPECTED_EXIT) \
    BOOST_AUTO_TEST_CASE(TEST_NAME) \
    { \
        boost::process::ipstream cpplox_out; \
        boost::process::ipstream cpplox_err; \
        const auto exit_code = boost::process::system( \
            "cpploxbc ../src/test/lox/" TEST_FILE, \
            boost::process::std_out > cpplox_out, \
            boost::process::std_err > cpplox_err \
        ); \
        std::string actual_out {std::istreambuf_iterator<char>{cpplox_out}, {}}; \
        std::string actual_err {std::istreambuf_iterator<char>{cpplox_err}, {}}; \
        \
        BOOST_TEST(actual_out == EXPECTED_OUT); \
        BOOST_TEST(actual_err == EXPECTED_ERR); \
        BOOST_TEST(exit_code == EXPECTED_EXIT); \
    }

MOTTS_LOX_MAKE_TEST_CASE(empty_file_test, "empty_file.lox", "", "", 0)
MOTTS_LOX_MAKE_TEST_CASE(precedence_test, "precedence.lox", "14\n8\n4\n0\ntrue\ntrue\ntrue\ntrue\n0\n0\n0\n0\n4\n", "", 0)
MOTTS_LOX_MAKE_TEST_CASE(unexpected_character_test, "unexpected_character.lox", "", "[Line 3] Error: Unexpected character \"|\".\n", 1)

MOTTS_LOX_MAKE_TEST_CASE(assignment_associativity_test, "assignment/associativity.lox", "c\nc\nc\n", "", 0)
MOTTS_LOX_MAKE_TEST_CASE(assignment_global_test, "assignment/global.lox", "before\nafter\narg\narg\n", "", 0)
MOTTS_LOX_MAKE_TEST_CASE(assignment_grouping_test, "assignment/grouping.lox", "", "[Line 2] Error at \"=\": Invalid assignment target.\n", 1)
MOTTS_LOX_MAKE_TEST_CASE(assignment_infix_operator_test, "assignment/infix_operator.lox", "", "[Line 3] Error at \"=\": Invalid assignment target.\n", 1)
MOTTS_LOX_MAKE_TEST_CASE(assignment_local_test, "assignment/local.lox", "before\nafter\narg\narg\n", "", 0)
MOTTS_LOX_MAKE_TEST_CASE(assignment_prefix_operator_test, "assignment/prefix_operator.lox", "", "[Line 2] Error at \"=\": Invalid assignment target.\n", 1)
MOTTS_LOX_MAKE_TEST_CASE(assignment_syntax_test, "assignment/syntax.lox", "var\nvar\n", "", 0)
MOTTS_LOX_MAKE_TEST_CASE(assignment_to_this_test, "assignment/to_this.lox", "", "[Line 3] Error at \"=\": Invalid assignment target.\n", 1)
MOTTS_LOX_MAKE_TEST_CASE(assignment_undefined_test, "assignment/undefined.lox", "", "[Line 1] Error: Undefined variable \"unknown\".\n", 1)

MOTTS_LOX_MAKE_TEST_CASE(block_empty_test, "block/empty.lox", "ok\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(block_scope_test, "block/scope.lox", "inner\nouter\n", "", 0);

MOTTS_LOX_MAKE_TEST_CASE(bool_equality_test, "bool/equality.lox", "true\nfalse\nfalse\ntrue\n" "false\nfalse\nfalse\nfalse\nfalse\n" "false\ntrue\ntrue\nfalse\n" "true\ntrue\ntrue\ntrue\ntrue\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(bool_not_test, "bool/not.lox", "false\ntrue\ntrue\n", "", 0);

MOTTS_LOX_MAKE_TEST_CASE(call_bool_test, "call/bool.lox", "", "[Line 1] Error at \"true\": Can only call functions and classes.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(call_nil_test, "call/nil.lox", "", "[Line 1] Error at \"nil\": Can only call functions and classes.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(call_num_test, "call/num.lox", "", "[Line 1] Error at \"123\": Can only call functions and classes.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(call_object_test, "call/object.lox", "", "[Line 4] Error at \"foo\": Can only call functions and classes.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(call_string_test, "call/string.lox", "", "[Line 1] Error at \"\"str\"\": Can only call functions and classes.\n", 1);

MOTTS_LOX_MAKE_TEST_CASE(class_empty_test, "class/empty.lox", "<class Foo>\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(class_inherited_method_test, "class/inherited_method.lox", "in foo\nin bar\nin baz\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(class_local_reference_self_test, "class/local_reference_self.lox", "<class Foo>\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(class_reference_self_test, "class/reference_self.lox", "<class Foo>\n", "", 0);

MOTTS_LOX_MAKE_TEST_CASE(closure_assign_to_closure_test, "closure/assign_to_closure.lox", "local\nafter f\nafter f\nafter g\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(closure_assign_to_shadowed_later_test, "closure/assign_to_shadowed_later.lox", "inner\nassigned\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(closure_close_over_function_parameter_test, "closure/close_over_function_parameter.lox", "param\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(closure_close_over_method_parameter_test, "closure/close_over_method_parameter.lox", "param\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(closure_close_over_later_variable_test, "closure/close_over_later_variable.lox", "b\na\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(closure_closed_closure_in_function_test, "closure/closed_closure_in_function.lox", "local\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(closure_nested_closure_test, "closure/nested_closure.lox", "a\nb\nc\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(closure_open_closure_in_function_test, "closure/open_closure_in_function.lox", "local\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(closure_reference_closure_multiple_times_test, "closure/reference_closure_multiple_times.lox", "a\na\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(closure_reuse_closure_slot_test, "closure/reuse_closure_slot.lox", "a\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(closure_shadow_closure_with_local_test, "closure/shadow_closure_with_local.lox", "closure\nshadow\nclosure\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(closure_unused_closure_test, "closure/unused_closure.lox", "ok\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(closure_unused_later_closure_test, "closure/unused_later_closure.lox", "a\n", "", 0);

MOTTS_LOX_MAKE_TEST_CASE(comments_line_at_eof_test, "comments/line_at_eof.lox", "ok\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(comments_only_line_comment_test, "comments/only_line_comment.lox", "", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(comments_only_line_comment_and_line_test, "comments/only_line_comment_and_line.lox", "", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(comments_unicode_test, "comments/unicode.lox", "ok\n", "", 0);

MOTTS_LOX_MAKE_TEST_CASE(constructor_arguments_test, "constructor/arguments.lox", "init\n1\n2\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(constructor_call_init_explicitly_test, "constructor/call_init_explicitly.lox", "Foo.init(one)\nFoo.init(two)\n<instance Foo>\ninit\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(constructor_default_test, "constructor/default.lox", "<instance Foo>\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(constructor_default_arguments_test, "constructor/default_arguments.lox", "", "[Line 3] Error at \"Foo\": Expected 0 arguments but got 3.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(constructor_early_return_test, "constructor/early_return.lox", "init\n<instance Foo>\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(constructor_extra_arguments_test, "constructor/extra_arguments.lox", "", "[Line 8] Error at \"Foo\": Expected 2 arguments but got 4.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(constructor_init_not_method_test, "constructor/init_not_method.lox", "not initializer\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(constructor_missing_arguments_test, "constructor/missing_arguments.lox", "", "[Line 5] Error at \"Foo\": Expected 2 arguments but got 1.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(constructor_return_in_nested_function_test, "constructor/return_in_nested_function.lox", "bar\n<instance Foo>\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(constructor_return_value_test, "constructor/return_value.lox", "", "[Line 3] Error at \"return\": Can't return a value from an initializer.\n", 1);

MOTTS_LOX_MAKE_TEST_CASE(field_call_function_field_test, "field/call_function_field.lox", "bar\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(field_call_nonfunction_field_test, "field/call_nonfunction_field.lox", "", "[Line 6] Error at \"bar\": Can only call functions and classes.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(field_get_and_set_method_test, "field/get_and_set_method.lox", "other\nmethod\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(field_get_on_bool_test, "field/get_on_bool.lox", "", "[Line 1] Error at \"foo\": Only instances have fields.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(field_get_on_class_test, "field/get_on_class.lox", "", "[Line 2] Error at \"bar\": Only instances have fields.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(field_get_on_function_test, "field/get_on_function.lox", "", "[Line 3] Error at \"bar\": Only instances have fields.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(field_get_on_nil_test, "field/get_on_nil.lox", "", "[Line 1] Error at \"foo\": Only instances have fields.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(field_get_on_num_test, "field/get_on_num.lox", "", "[Line 1] Error at \"foo\": Only instances have fields.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(field_get_on_string_test, "field/get_on_string.lox", "", "[Line 1] Error at \"foo\": Only instances have fields.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(field_many_test, "field/many.lox",
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
    "yuzu\n",
    "", 0
);
MOTTS_LOX_MAKE_TEST_CASE(field_method_test, "field/method.lox", "got method\narg\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(field_method_binds_this_test, "field/method_binds_this.lox", "foo1\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(field_on_instance_test, "field/on_instance.lox", "bar value\nbaz value\nbar value\nbaz value\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(field_set_evaluation_order_test, "field/set_evaluation_order.lox", "", "[Line 2] Error: Undefined variable \"undefined2\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(field_set_on_bool_test, "field/set_on_bool.lox", "", "[Line 1] Error at \"=\": Invalid assignment target.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(field_set_on_class_test, "field/set_on_class.lox", "", "[Line 2] Error at \"bar\": Only instances have fields.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(field_set_on_function_test, "field/set_on_function.lox", "", "[Line 3] Error at \"bar\": Only instances have fields.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(field_set_on_nil_test, "field/set_on_nil.lox", "", "[Line 1] Error at \"=\": Invalid assignment target.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(field_set_on_num_test, "field/set_on_num.lox", "", "[Line 1] Error at \"=\": Invalid assignment target.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(field_set_on_string_test, "field/set_on_string.lox", "", "[Line 1] Error at \"=\": Invalid assignment target.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(field_undefined_test, "field/undefined.lox", "", "[Line 4] Error: Undefined property \"bar\".\n", 1);

MOTTS_LOX_MAKE_TEST_CASE(for_class_in_body_test, "for/class_in_body.lox", "", "[Line 2] Error: Unexpected token \"class\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(for_closure_in_body_test, "for/closure_in_body.lox", "1\n2\n3\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(for_return_closure_test, "for/return_closure.lox", "i\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(for_return_inside_test, "for/return_inside.lox", "i\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(for_scope_test, "for/scope.lox", "0\n-1\nafter\n0\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(for_statement_condition_test, "for/statement_condition.lox", "", "[Line 3] Error: Unexpected token \"{\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(for_statement_increment_test, "for/statement_increment.lox", "", "[Line 2] Error: Unexpected token \"{\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(for_statement_initializer_test, "for/statement_initializer.lox", "", "[Line 3] Error: Unexpected token \"{\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(for_syntax_test, "for/syntax.lox", "1\n2\n3\n0\n1\n2\ndone\n0\n1\n0\n1\n2\n0\n1\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(for_var_in_body_test, "for/var_in_body.lox", "", "[Line 2] Error: Unexpected token \"var\".\n", 1);

MOTTS_LOX_MAKE_TEST_CASE(function_body_must_be_block_test, "function/body_must_be_block.lox", "", "[Line 3] Error at \"123\": Expected LEFT_BRACE.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(function_empty_body_test, "function/empty_body.lox", "nil\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(function_extra_arguments_test, "function/extra_arguments.lox", "", "[Line 6] Error at \"f\": Expected 2 arguments but got 4.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(function_local_mutual_recursion_test, "function/local_mutual_recursion.lox", "", "[Line 4] Error: Undefined variable \"isOdd\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(function_local_recursion_test, "function/local_recursion.lox", "21\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(function_missing_arguments_test, "function/missing_arguments.lox", "", "[Line 3] Error at \"f\": Expected 2 arguments but got 1.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(function_missing_comma_in_parameters_test, "function/missing_comma_in_parameters.lox", "", "[Line 3] Error at \"c\": Expected RIGHT_PAREN.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(function_mutual_recursion_test, "function/mutual_recursion.lox", "true\ntrue\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(function_parameters_test, "function/parameters.lox", "0\n1\n3\n6\n10\n15\n21\n28\n36\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(function_print_test, "function/print.lox", "<fn foo>\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(function_recursion_test, "function/recursion.lox", "21\n", "", 0);

MOTTS_LOX_MAKE_TEST_CASE(if_class_in_else_test, "if/class_in_else.lox", "", "[Line 2] Error: Unexpected token \"class\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(if_class_in_then_test, "if/class_in_then.lox", "", "[Line 2] Error: Unexpected token \"class\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(if_dangling_else_test, "if/dangling_else.lox", "good\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(if_else_test, "if/else.lox", "good\ngood\nblock\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(if_if_test, "if/if.lox", "good\nblock\ntrue\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(if_truth_test, "if/truth.lox", "false\nnil\ntrue\n0\nempty\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(if_var_in_else_test, "if/var_in_else.lox", "", "[Line 2] Error: Unexpected token \"var\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(if_var_in_then_test, "if/var_in_then.lox", "", "[Line 2] Error: Unexpected token \"var\".\n", 1);

MOTTS_LOX_MAKE_TEST_CASE(inheritance_inherit_from_function_test, "inheritance/inherit_from_function.lox", "", "[Line 3] Error at \"foo\": Superclass must be a class.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(inheritance_inherit_from_nil_test, "inheritance/inherit_from_nil.lox", "", "[Line 2] Error at \"Nil\": Superclass must be a class.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(inheritance_inherit_from_number_test, "inheritance/inherit_from_number.lox", "", "[Line 2] Error at \"Number\": Superclass must be a class.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(inheritance_inherit_methods_test, "inheritance/inherit_methods.lox", "foo\nbar\nbar\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(inheritance_parenthesized_superclass_test, "inheritance/parenthesized_superclass.lox", "", "[Line 4] Error at \"(\": Expected IDENTIFIER.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(inheritance_set_fields_from_base_class_test, "inheritance/set_fields_from_base_class.lox", "foo 1\nfoo 2\nbar 1\nbar 2\nbar 1\nbar 2\n", "", 0);

MOTTS_LOX_MAKE_TEST_CASE(logical_operator_and_test, "logical_operator/and.lox", "false\n1\nfalse\ntrue\n3\ntrue\nfalse\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(logical_operator_and_truth_test, "logical_operator/and_truth.lox", "false\nnil\nok\nok\nok\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(logical_operator_or_test, "logical_operator/or.lox", "1\n1\ntrue\nfalse\nfalse\nfalse\ntrue\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(logical_operator_or_truth_test, "logical_operator/or_truth.lox", "ok\nok\ntrue\n0\ns\n", "", 0);

MOTTS_LOX_MAKE_TEST_CASE(method_arity_test, "method/arity.lox", "no args\n1\n3\n6\n10\n15\n21\n28\n36\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(method_empty_block_test, "method/empty_block.lox", "nil\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(method_extra_arguments_test, "method/extra_arguments.lox", "", "[Line 8] Error at \"method\": Expected 2 arguments but got 4.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(method_missing_arguments_test, "method/missing_arguments.lox", "", "[Line 5] Error at \"method\": Expected 2 arguments but got 1.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(method_not_found_test, "method/not_found.lox", "", "[Line 3] Error: Undefined property \"unknown\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(method_refer_to_name_test, "method/refer_to_name.lox", "", "[Line 3] Error: Undefined variable \"method\".\n", 1);

MOTTS_LOX_MAKE_TEST_CASE(nil_literal_test, "nil/literal.lox", "nil\n", "", 0);

MOTTS_LOX_MAKE_TEST_CASE(number_decimal_point_at_eof_test, "number/decimal_point_at_eof.lox", "", "[Line 2] Error at \"EOF\": Expected IDENTIFIER.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(number_leading_dot_test, "number/leading_dot.lox", "", "[Line 2] Error: Unexpected token \".\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(number_literals_test, "number/literals.lox", "123\n987654\n0\n-0\n123.456\n-0.001\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(number_trailing_dot_test, "number/trailing_dot.lox", "", "[Line 2] Error at \";\": Expected IDENTIFIER.\n", 1);

MOTTS_LOX_MAKE_TEST_CASE(operator_add_test, "operator/add.lox", "579\nstring\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(operator_add_bool_nil_test, "operator/add_bool_nil.lox", "", "[Line 1] Error at \"+\": Operands must be two numbers or two strings.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_add_bool_num_test, "operator/add_bool_num.lox", "", "[Line 1] Error at \"+\": Operands must be two numbers or two strings.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_add_bool_string_test, "operator/add_bool_string.lox", "", "[Line 1] Error at \"+\": Operands must be two numbers or two strings.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_add_nil_nil_test, "operator/add_nil_nil.lox", "", "[Line 1] Error at \"+\": Operands must be two numbers or two strings.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_add_num_nil_test, "operator/add_num_nil.lox", "", "[Line 1] Error at \"+\": Operands must be two numbers or two strings.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_add_string_nil_test, "operator/add_string_nil.lox", "", "[Line 1] Error at \"+\": Operands must be two numbers or two strings.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_comparison_test, "operator/comparison.lox", "true\nfalse\nfalse\ntrue\ntrue\nfalse\nfalse\nfalse\ntrue\nfalse\ntrue\ntrue\nfalse\nfalse\nfalse\nfalse\ntrue\ntrue\ntrue\ntrue\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(operator_divide_test, "operator/divide.lox", "4\n1\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(operator_divide_nonnum_num_test, "operator/divide_nonnum_num.lox", "", "[Line 1] Error at \"/\": Operands must be numbers.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_divide_num_nonnum_test, "operator/divide_num_nonnum.lox", "", "[Line 1] Error at \"/\": Operands must be numbers.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_equals_test, "operator/equals.lox", "true\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\nfalse\nfalse\nfalse\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(operator_equals_class_test, "operator/equals_class.lox", "true\nfalse\nfalse\ntrue\nfalse\nfalse\nfalse\nfalse\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(operator_equals_method_test, "operator/equals_method.lox", "true\nfalse\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(operator_greater_nonnum_num_test, "operator/greater_nonnum_num.lox", "", "[Line 1] Error at \">\": Operands must be numbers.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_greater_num_nonnum_test, "operator/greater_num_nonnum.lox", "", "[Line 1] Error at \">\": Operands must be numbers.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_greater_or_equal_nonnum_num_test, "operator/greater_or_equal_nonnum_num.lox", "", "[Line 1] Error at \">=\": Operands must be numbers.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_greater_or_equal_num_nonnum_test, "operator/greater_or_equal_num_nonnum.lox", "", "[Line 1] Error at \">=\": Operands must be numbers.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_less_nonnum_num_test, "operator/less_nonnum_num.lox", "", "[Line 1] Error at \"<\": Operands must be numbers.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_less_num_nonnum_test, "operator/less_num_nonnum.lox", "", "[Line 1] Error at \"<\": Operands must be numbers.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_less_or_equal_nonnum_num_test, "operator/less_or_equal_nonnum_num.lox", "", "[Line 1] Error at \"<=\": Operands must be numbers.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_less_or_equal_num_nonnum_test, "operator/less_or_equal_num_nonnum.lox", "", "[Line 1] Error at \"<=\": Operands must be numbers.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_multiply_test, "operator/multiply.lox", "15\n3.702\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(operator_multiply_nonnum_num_test, "operator/multiply_nonnum_num.lox", "", "[Line 1] Error at \"*\": Operands must be numbers.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_multiply_num_nonnum_test, "operator/multiply_num_nonnum.lox", "", "[Line 1] Error at \"*\": Operands must be numbers.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_negate_test, "operator/negate.lox", "-3\n3\n-3\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(operator_negate_nonnum_test, "operator/negate_nonnum.lox", "", "[Line 1] Error at \"-\": Operand must be a number.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_not_test, "operator/not.lox", "false\ntrue\ntrue\nfalse\nfalse\ntrue\nfalse\nfalse\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(operator_not_class_test, "operator/not_class.lox", "false\nfalse\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(operator_not_equals_test, "operator/not_equals.lox", "false\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\ntrue\ntrue\ntrue\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(operator_subtract_test, "operator/subtract.lox", "1\n0\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(operator_subtract_nonnum_num_test, "operator/subtract_nonnum_num.lox", "", "[Line 1] Error at \"-\": Operands must be numbers.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(operator_subtract_num_nonnum_test, "operator/subtract_num_nonnum.lox", "", "[Line 1] Error at \"-\": Operands must be numbers.\n", 1);

MOTTS_LOX_MAKE_TEST_CASE(print_missing_argument_test, "print/missing_argument.lox", "", "[Line 2] Error: Unexpected token \";\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(regression_40_test, "regression/40.lox", "false\n", "", 0);

MOTTS_LOX_MAKE_TEST_CASE(return_after_else_test, "return/after_else.lox", "ok\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(return_after_if_test, "return/after_if.lox", "ok\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(return_after_while_test, "return/after_while.lox", "ok\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(return_at_top_level_test, "return/at_top_level.lox", "", "[Line 1] Error at \"return\": Can't return from top-level code.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(return_in_function_test, "return/in_function.lox", "ok\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(return_in_method_test, "return/in_method.lox", "ok\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(return_return_nil_if_no_value_test, "return/return_nil_if_no_value.lox", "nil\n", "", 0);

MOTTS_LOX_MAKE_TEST_CASE(string_error_after_multiline_test, "string/error_after_multiline.lox", "", "[Line 7] Error: Undefined variable \"err\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(string_literals_test, "string/literals.lox", "()\na string\nA~¶Þॐஃ\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(string_multiline_test, "string/multiline.lox", "1\n2\n3\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(string_unterminated_test, "string/unterminated.lox", "", "[Line 2] Error: Unterminated string.\n", 1);

MOTTS_LOX_MAKE_TEST_CASE(super_bound_method_test, "super/bound_method.lox", "A.method(arg)\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(super_call_other_method_test, "super/call_other_method.lox", "Derived.bar()\nBase.foo()\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(super_call_same_method_test, "super/call_same_method.lox", "Derived.foo()\nBase.foo()\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(super_closure_test, "super/closure.lox", "Base\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(super_constructor_test, "super/constructor.lox", "Derived.init()\nBase.init(a, b)\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(super_extra_arguments_test, "super/extra_arguments.lox", "Derived.foo()\n", "[Line 10] Error at \"super\": Expected 2 arguments but got 4.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(super_indirectly_inherited_test, "super/indirectly_inherited.lox", "C.foo()\nA.foo()\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(super_missing_arguments_test, "super/missing_arguments.lox", "", "[Line 9] Error at \"super\": Expected 2 arguments but got 1.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(super_no_superclass_bind_test, "super/no_superclass_bind.lox", "", "[Line 3] Error: Undefined variable \"super\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(super_no_superclass_call_test, "super/no_superclass_call.lox", "", "[Line 3] Error: Undefined variable \"super\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(super_no_superclass_method_test, "super/no_superclass_method.lox", "", "[Line 5] Error: Undefined property \"doesNotExist\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(super_parenthesized_test, "super/parenthesized.lox", "", "[Line 8] Error at \")\": Expected DOT.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(super_reassign_superclass_test, "super/reassign_superclass.lox", "Base.method()\nBase.method()\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(super_super_at_top_level_test, "super/super_at_top_level.lox", "", "[Line 1] Error: Undefined variable \"this\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(super_super_in_closure_in_inherited_method_test, "super/super_in_closure_in_inherited_method.lox", "A\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(super_super_in_inherited_method_test, "super/super_in_inherited_method.lox", "A\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(super_super_in_top_level_function_test, "super/super_in_top_level_function.lox", "", "[Line 2] Error: Undefined variable \"this\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(super_super_without_dot_test, "super/super_without_dot.lox", "", "[Line 6] Error at \";\": Expected DOT.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(super_super_without_name_test, "super/super_without_name.lox", "", "[Line 5] Error at \";\": Expected IDENTIFIER.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(super_this_in_superclass_method_test, "super/this_in_superclass_method.lox", "a\nb\n", "", 0);

MOTTS_LOX_MAKE_TEST_CASE(this_closure_test, "this/closure.lox", "Foo\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(this_nested_class_test, "this/nested_class.lox", "<instance Outer>\n<instance Outer>\n<instance Inner>\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(this_nested_closure_test, "this/nested_closure.lox", "Foo\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(this_this_at_top_level_test, "this/this_at_top_level.lox", "", "[Line 1] Error: Undefined variable \"this\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(this_this_in_method_test, "this/this_in_method.lox", "baz\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(this_this_in_top_level_function_test, "this/this_in_top_level_function.lox", "", "[Line 2] Error: Undefined variable \"this\".\n", 1);

MOTTS_LOX_MAKE_TEST_CASE(variable_collide_with_parameter_test, "variable/collide_with_parameter.lox", "", "[Line 2] Error at \"a\": Identifier with this name already declared in this scope.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(variable_duplicate_local_test, "variable/duplicate_local.lox", "", "[Line 3] Error at \"a\": Identifier with this name already declared in this scope.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(variable_duplicate_parameter_test, "variable/duplicate_parameter.lox", "", "[Line 2] Error at \"arg\": Identifier with this name already declared in this scope.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(variable_early_bound_test, "variable/early_bound.lox", "outer\nouter\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(variable_in_middle_of_block_test, "variable/in_middle_of_block.lox", "a\na b\na c\na b d\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(variable_in_nested_block_test, "variable/in_nested_block.lox", "outer\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(variable_local_from_method_test, "variable/local_from_method.lox", "variable\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(variable_redeclare_global_test, "variable/redeclare_global.lox", "nil\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(variable_redefine_global_test, "variable/redefine_global.lox", "2\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(variable_scope_reuse_in_different_blocks_test, "variable/scope_reuse_in_different_blocks.lox", "first\nsecond\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(variable_shadow_and_local_test, "variable/shadow_and_local.lox", "outer\ninner\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(variable_shadow_global_test, "variable/shadow_global.lox", "shadow\nglobal\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(variable_shadow_local_test, "variable/shadow_local.lox", "shadow\nlocal\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(variable_undefined_global_test, "variable/undefined_global.lox", "", "[Line 1] Error: Undefined variable \"notDefined\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(variable_undefined_local_test, "variable/undefined_local.lox", "", "[Line 2] Error: Undefined variable \"notDefined\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(variable_uninitialized_test, "variable/uninitialized.lox", "nil\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(variable_unreached_undefined_test, "variable/unreached_undefined.lox", "ok\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(variable_use_false_as_var_test, "variable/use_false_as_var.lox", "", "[Line 2] Error at \"false\": Expected IDENTIFIER.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(variable_use_global_in_initializer_test, "variable/use_global_in_initializer.lox", "value\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(variable_use_local_in_initializer_test, "variable/use_local_in_initializer.lox", "", "[Line 3] Error at \"a\": Cannot read local variable in its own initializer.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(variable_use_nil_as_var_test, "variable/use_nil_as_var.lox", "", "[Line 2] Error at \"nil\": Expected IDENTIFIER.\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(variable_use_this_as_var_test, "variable/use_this_as_var.lox", "", "[Line 2] Error at \"this\": Expected IDENTIFIER.\n", 1);

MOTTS_LOX_MAKE_TEST_CASE(while_class_in_body_test, "while/class_in_body.lox", "", "[Line 2] Error: Unexpected token \"class\".\n", 1);
MOTTS_LOX_MAKE_TEST_CASE(while_closure_in_body_test, "while/closure_in_body.lox", "1\n2\n3\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(while_return_closure_test, "while/return_closure.lox", "i\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(while_return_inside_test, "while/return_inside.lox", "i\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(while_syntax_test, "while/syntax.lox", "1\n2\n3\n0\n1\n2\n", "", 0);
MOTTS_LOX_MAKE_TEST_CASE(while_var_in_body_test, "while/var_in_body.lox", "", "[Line 2] Error: Unexpected token \"var\".\n", 1);
