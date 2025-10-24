#include <stdbool.h>
#define YUKTI_TEST_IMPLEMENTATION
#include "yukti.h"

#define CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE 10 // A small enough number for easy testing
#define CARGS_MAX_NAME_LEN_OVERRIDE        CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE
#define CARGS_MAX_DESCRIPTION_LEN_OVERRIDE CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE
#define CARGS_IMPLEMENTATION
#include "../cargs.h"

/**************************************************************************************************
 * Requirements:
 * cargs_parse_input
 *  - [REQ: 2 ] A conditional arg is active if the predicate function returns true.
 *  - [REQ: 4 ] Arguments must have CARGS__ARGUMENT_PREFIX_CHAR as the first char.
 *  - [REQ: 5 ] Fail if no value is provided for active, non-optional args.
 *  - [REQ: 6 ] Fail if value is provided for non-active args.
 *  - [REQ: 7 ] Fail if value is provided for unknown arg.
 *  - [REQ: 8 ] Help arg should stop further arg parsing and pass.
 *  - [REQ: 9 ] For non list args, provided value is parsed as per Interface type.
 *  - [REQ: 13] For list args, all provided values are parsed as per Interface type.
 * cargs_add_arg
 *  - [REQ: 10] For list arg, multiple values of any number can be accessed by the pointer.
 *  - [REQ: 11] For non-list arg, value can be accessed by the pointer.
 *  - [REQ: 14] A non-list arg with a provided default value is treated as optional.
 *  - [REQ: 15] For optional args, the default value is accessed by the pointer if no arg was given.
 * General
 *  - [REQ: 12] All string inputs must have some cap on its length when accessing.
 *
 * |-------------------|----------------------------------------------|---------------------------|
 * | FUT               | Requirement/Test case                        | Test function name        |
 * |-------------------|----------------------------------------------|---------------------------|
 * | cargs_add_arg,    | * [REQ: 11], [REQ: 9], [REQ: 4]              |non_list_arguments_parsing |
 * | cargs_parse_input |                                              |                           |
 * |                   | Argument values of were provided in cmd line |                           |
 * |                   | Parsing should pass and results match with   |                           |
 * |                   | input.                                       |                           |
 * |-------------------|----------------------------------------------|---------------------------|
 * | cargs_parse_input | * [REQ: 5]                                   |required_arguments         |
 * |                   |                                              |                           |
 * |                   |----------------------------------------------|---------------------------|
 * |                   | Required arguments where provided.           | Test# 1                   |
 * |                   | Parsing should pass.                         |                           |
 * |                   |----------------------------------------------|---------------------------|
 * |                   | Required arguments where not provided.       | Test# 2                   |
 * |                   | Parsing should fail.                         |                           |
 * |-------------------|----------------------------------------------|---------------------------|
 * | cargs_parse_input | * [REQ: 8]                                   |help_argument_present      |
 * |                   |                                              |                           |
 * |                   | Help argument is provided when a required    |                           |
 * |                   | arg was not provided. Parsing should pass.   |                           |
 * |-------------------|----------------------------------------------|---------------------------|
 * | cargs_add_arg,    | * [REQ: 10], [REQ: 13]                       |list_type_argument_success |
 * | CARGS_LISTOF      |                                              |                           |
 * | cargs_parse_input |                                              |                           |
 * |                   |                                              |                           |
 * |                   | Multiple values were passed for each argument|                           |
 * |                   | Parsing should pass.                         |                           |
 * |-------------------|----------------------------------------------|---------------------------|
 * | cargs_parse_input | * [REQ: 12]                                  |arg_value_length_clamping  |
 * |                   |                                              |                           |
 * |                   | Argument value in command line has excess    |                           |
 * |                   | characters. Parsing should pass.             |                           |
 * |                   | Parsing should pass.                         |                           |
 * |-------------------|----------------------------------------------|---------------------------|
 * | cargs_parse_input | * [REQ: 12]                                  |arg_name_length_clamping   |
 * |                   |                                              |                           |
 * |                   | Argument name in command line has excess     |                           |
 * |                   | characters. Parsing should pass.             |                           |
 * |-------------------|----------------------------------------------|---------------------------|
 * | cargs_add_arg,    | * [REQ: 5], [REQ: 14], [REQ: 15]             |default_values             |
 * | cargs_parse_input |                                              |                           |
 * |                   |                                              |                           |
 * |                   | Default values for arguments where provided  |                           |
 * |                   | and none provided in command line.           |                           |
 * |                   | Parsing should pass.                         |                           |
 * |-------------------|----------------------------------------------|---------------------------|
 * | cargs_add_arg,    | * [REQ: 5], [REQ: 15], [REQ: 9]              |default_values_override    |
 * | cargs_parse_input |                                              |                           |
 * |                   |                                              |                           |
 * |                   | Default values for arguments where provided  |                           |
 * |                   | and were also provided in command line.      |                           |
 * |                   | Parsing should pass.                         |                           |
 * |-------------------|----------------------------------------------|---------------------------|
 * | cargs_parse_input | * [REQ: 7]                                   |unknown_argument_in_cl     |
 * |                   |                                              |                           |
 * |                   | Unknown argument found at the time of parsing|                           |
 * |                   | command line.                                |                           |
 * |                   | Parsing should fail.                         |                           |
 * |-------------------|----------------------------------------------|---------------------------|
 * | cargs_parse_input | * [REQ: 2], [REQ: 6]                         |inactive_argument_in_cl    |
 * |                   |----------------------------------------------|---------------------------|
 * |                   | Known argument provided for inactive arg     | Test# 1                   |
 * |                   | Parsing should fail.                         |                           |
 * |                   |----------------------------------------------|---------------------------|
 * |                   | Known argument provided for active arg       | Test# 2                   |
 * |                   | Parsing should pass.                         |                           |
 * |-------------------|----------------------------------------------|---------------------------|
 **************************************************************************************************/

#define ARRAY_LEN(a) (sizeof (a) / sizeof (a[0]))

static bool always_true()
{
    return true;
}

static bool always_false()
{
    return false;
}

YT_TEST (cargs, non_list_arguments_parsing)
{
    char* a   = cargs_add_arg ("A", "1st arg", String, NULL);
    int* b    = cargs_add_arg ("B", "2nd arg", Integer, NULL);
    bool* c   = cargs_add_arg ("C", "3rd arg", Boolean, NULL);
    double* d = cargs_add_arg ("D", "4th arg", Double, NULL);

    char* argv[] = { "dummy", "-A", "abc", "-B", "1", "-C", "true", "-D", "12.84", NULL };
    YT_EQ_SCALAR (true, cargs_parse_input (ARRAY_LEN (argv), argv));

    YT_EQ_STRING (a, "abc");
    YT_EQ_SCALAR (*b, 1);
    YT_EQ_SCALAR (*c, true);
    YT_EQ_DOUBLE_REL (*d, 12.84, 0.001);

    YT_END();
}

YT_TESTP (cargs, required_arguments, bool)
{
    bool is_required_args_provided = YT_ARG_0();

    cargs_add_arg ("A", "1st arg", String, NULL);

    if (is_required_args_provided) {
        char* argv[] = { "dummy", "-A", "abc", NULL };
        YT_EQ_SCALAR (true, cargs_parse_input (ARRAY_LEN (argv), argv));
    } else {
        char* argv[] = { "dummy", NULL };
        YT_EQ_SCALAR (false, cargs_parse_input (ARRAY_LEN (argv), argv));
    }

    YT_END();
}

YT_TEST (cargs, list_type_argument_success)
{
    char* argv[] = { "dummy", "-A", "abc", "-B", "1", "2", "3", "-C", "true", NULL };

    char* a            = cargs_add_arg ("A", "1st arg", String, NULL);
    Cargs_ArrayList* b = cargs_add_arg ("B", "2nd arg", CARGS_LISTOF (Integer), NULL);
    char* c            = cargs_add_arg ("C", "3rd arg", Boolean, NULL);

    YT_EQ_SCALAR (true, cargs_parse_input (ARRAY_LEN (argv), argv));

    YT_EQ_STRING (a, "abc");
    YT_EQ_SCALAR (*c, true);
    YT_EQ_SCALAR (b->len, 3U);
    YT_EQ_SCALAR (((int*)b->buffer)[0], 1);
    YT_EQ_SCALAR (((int*)b->buffer)[1], 2);
    YT_EQ_SCALAR (((int*)b->buffer)[2], 3);

    YT_END();
}

YT_TEST (cargs, arg_value_length_clamping)
{
    char* argv[] = { "dummy", "-A", "123456789ABCD", NULL };

    char* a = cargs_add_arg ("A", "1st arg", String, NULL);
    YT_EQ_SCALAR (true, cargs_parse_input (ARRAY_LEN (argv), argv));

    // CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE number of non-null characters
    YT_EQ_STRING (a, "123456789A");
    YT_END();
}

YT_TEST (cargs, arg_name_length_clamping)
{
    char* a = cargs_add_arg ("A23456789", "1st arg", String, NULL);

    // Argument name is clamped to CARGS__MAX_NAME_LEN_OVERRIDE number of non-null characters
    char* argv[] = { "dummy", "-A23456789BCD", "123", NULL };

    YT_EQ_SCALAR (true, cargs_parse_input (ARRAY_LEN (argv), argv));
    YT_EQ_STRING (a, "123");

    YT_END();
}

YT_TEST (cargs, help_argument_present)
{
    // When help flag is found during parsing, the check to see if all required arguments were
    // provided is not done, thus this test has one required argument missing in the command line
    // but has one Help arg and as a result cargs_parse_input would pass this time.

    char* argv[] = { "dummy", "-A", "abc", "-C", "true", "-D", "12.84", "-H", NULL };

    cargs_add_arg ("A", "1st arg", String, NULL);
    cargs_add_arg ("B", "2nd arg", Integer, NULL);
    cargs_add_arg ("C", "3rd arg", Boolean, NULL);
    cargs_add_arg ("D", "4th arg", Double, NULL);
    cargs_add_arg ("H", "Help arg", Help, "false");

    YT_EQ_SCALAR (true, cargs_parse_input (ARRAY_LEN (argv), argv));

    YT_END();
}

YT_TEST (cargs, default_values)
{
    char* argv[] = { "dummy", NULL };

    char* a   = cargs_add_arg ("A", "1st arg", String, "bcd");
    int* b    = cargs_add_arg ("B", "2nd arg", Integer, "13");
    bool* c   = cargs_add_arg ("C", "3rd arg", Boolean, "false");
    double* d = cargs_add_arg ("D", "4th arg", Double, "18.52");

    YT_EQ_SCALAR (true, cargs_parse_input (ARRAY_LEN (argv), argv));

    YT_EQ_STRING (a, "bcd");
    YT_EQ_SCALAR (*b, 13);
    YT_EQ_SCALAR (*c, false);
    YT_EQ_DOUBLE_REL (*d, 18.52, 0.001);

    YT_END();
}

YT_TEST (cargs, default_value_override)
{
    char* argv[] = { "dummy", "-A", "def", "-B", "23", "-C", "true", "-D", "220.72", NULL };

    char* a   = cargs_add_arg ("A", "1st arg", String, "bcd");
    int* b    = cargs_add_arg ("B", "2nd arg", Integer, "13");
    bool* c   = cargs_add_arg ("C", "3rd arg", Boolean, "false");
    double* d = cargs_add_arg ("D", "4th arg", Double, "18.52");

    YT_EQ_SCALAR (true, cargs_parse_input (ARRAY_LEN (argv), argv));

    YT_EQ_STRING (a, "def");
    YT_EQ_SCALAR (*b, 23);
    YT_EQ_SCALAR (*c, true);
    YT_EQ_DOUBLE_REL (*d, 220.72, 0.001);

    YT_END();
}
YT_TEST (cargs, unknown_argument_in_cl)
{
    char* argv[] = { "dummy", "-B", "23", NULL };

    cargs_add_arg ("A", "1st arg", String, NULL);

    YT_EQ_SCALAR (false, cargs_parse_input (ARRAY_LEN (argv), argv));

    YT_END();
}
YT_TESTP (cargs, inactive_argument_in_cl, bool)
{
    bool is_inactive_arg_present_in_cl = YT_ARG_0();
    char* argv[]                       = { "dummy", "-A", NULL };

    if (is_inactive_arg_present_in_cl) {
        cargs_add_cond_arg (always_false, "A", "1st arg", Flag, "false");
        YT_EQ_SCALAR (false, cargs_parse_input (ARRAY_LEN (argv), argv));
    } else {
        cargs_add_cond_arg (always_true, "A", "1st arg", Flag, "false");
        YT_EQ_SCALAR (true, cargs_parse_input (ARRAY_LEN (argv), argv));
    }

    YT_END();
}

void yt_reset (void)
{
    cargs_cleanup();
}

int main (void)
{
    YT_INIT();
    non_list_arguments_parsing();
    required_arguments (2, YT_ARG (bool){ true, false });
    default_values();
    default_value_override();
    help_argument_present();
    arg_value_length_clamping();
    list_type_argument_success();
    unknown_argument_in_cl();
    arg_name_length_clamping();
    inactive_argument_in_cl (2, YT_ARG (bool){ true, false });
    YT_RETURN_WITH_REPORT();
}
