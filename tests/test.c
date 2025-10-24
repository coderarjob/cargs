#include <stdbool.h>
#define YUKTI_TEST_IMPLEMENTATION
#include "yukti.h"

#define CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE 10 // A small enough number for easy testing
#define CARGS_MAX_NAME_LEN_OVERRIDE        CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE
#define CARGS_MAX_DESCRIPTION_LEN_OVERRIDE CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE
#define CARGS_IMPLEMENTATION
#include "../cargs.h"


#define ARRAY_LEN(a) (sizeof(a)/sizeof(a[0]))

static bool always_true()
{
    return true;
}

static bool always_false()
{
    return false;
}

YT_TESTP (cargs, required_arguments, bool, bool)
{
    bool test_subargs      = YT_ARG_0();
    bool all_args_provided = YT_ARG_1();

    int* b    = NULL;
    bool* c   = NULL;
    double* d = NULL;

    char* a = cargs_add_arg ("A", "1st arg", String, NULL);
    if (!test_subargs) {
        b = cargs_add_arg ("B", "2nd arg", Integer, NULL);
        c = cargs_add_arg ("C", "3rd arg", Boolean, NULL);
        d = cargs_add_arg ("D", "4th arg", Double, NULL);
    } else {
        b = cargs_add_subarg (a, always_true, "B", "2nd arg", Integer, NULL);
        c = cargs_add_subarg (a, always_true, "C", "3rd arg", Boolean, NULL);
        d = cargs_add_subarg (a, always_true, "D", "4th arg", Double, NULL);
    }

    if (all_args_provided) {
        char* argv[] = { "dummy", "-A", "abc", "-B", "1", "-C", "true", "-D", "12.84", NULL };
        YT_EQ_SCALAR (true, cargs_parse_input (ARRAY_LEN (argv), argv));

        YT_EQ_STRING (a, "abc");
        YT_EQ_SCALAR (*b, 1);
        YT_EQ_SCALAR (*c, true);
        YT_EQ_DOUBLE_REL (*d, 12.84, 0.001);
    } else {
        char* argv[] = { "dummy", "-A", "abc", "-B", "1", "-D", "12.84", NULL };
        YT_EQ_SCALAR (false, cargs_parse_input (ARRAY_LEN (argv), argv));
    }

    YT_END();
}

YT_TEST (cargs, list_type_argument_success)
{
    char* argv[] = { "dummy", "-A", "abc", "-B", "1", "2", "3", "-C", "true", NULL };
    int argc     = sizeof (argv) / sizeof (argv[0]);

    char* a            = cargs_add_arg ("A", "1st arg", String, NULL);
    Cargs_ArrayList* b = cargs_add_arg ("B", "2nd arg", CARGS_LISTOF (Integer), NULL);
    char* c            = cargs_add_arg ("C", "3rd arg", Boolean, NULL);

    YT_EQ_SCALAR (true, cargs_parse_input (argc, argv));

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
    int argc     = sizeof (argv) / sizeof (argv[0]);

    char* a = cargs_add_arg ("A", "1st arg", String, NULL);
    YT_EQ_SCALAR (true, cargs_parse_input (argc, argv));

    // CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE number of non-null characters
    YT_EQ_STRING (a, "123456789A");
    YT_END();
}

YT_TEST (cargs, arg_name_length_clamping)
{
    char* a = cargs_add_arg ("A23456789", "1st arg", String, NULL);

    // Argument name is clamped to CARGS__MAX_NAME_LEN_OVERRIDE number of non-null characters
    char* argv[] = { "dummy", "-A23456789BCD", "123", NULL };
    int argc     = sizeof (argv) / sizeof (argv[0]);

    YT_EQ_SCALAR (true, cargs_parse_input (argc, argv));
    YT_EQ_STRING (a, "123");

    YT_END();
}

YT_TEST (cargs, help_argument_present)
{
    // When help flag is found during parsing, the check to see if all required arguments were
    // provided is not done, thus this test has one required argument missing in the command line
    // but has one Help arg and as a result cargs_parse_input would pass this time.

    char* argv[] = { "dummy", "-A", "abc", "-C", "true", "-D", "12.84", "-H", NULL };
    int argc     = sizeof (argv) / sizeof (argv[0]);

    cargs_add_arg ("A", "1st arg", String, NULL);
    cargs_add_arg ("B", "2nd arg", Integer, NULL);
    cargs_add_arg ("C", "3rd arg", Boolean, NULL);
    cargs_add_arg ("D", "4th arg", Double, NULL);
    cargs_add_arg ("H", "Help arg", Help, "false");

    YT_EQ_SCALAR (true, cargs_parse_input (argc, argv));

    YT_END();
}

YT_TEST (cargs, default_values)
{
    char* argv[] = { "dummy", NULL };
    int argc     = sizeof (argv) / sizeof (argv[0]);

    char* a   = cargs_add_arg ("A", "1st arg", String, "bcd");
    int* b    = cargs_add_arg ("B", "2nd arg", Integer, "13");
    bool* c   = cargs_add_arg ("C", "3rd arg", Boolean, "false");
    double* d = cargs_add_arg ("D", "4th arg", Double, "18.52");

    YT_EQ_SCALAR (true, cargs_parse_input (argc, argv));

    YT_EQ_STRING (a, "bcd");
    YT_EQ_SCALAR (*b, 13);
    YT_EQ_SCALAR (*c, false);
    YT_EQ_DOUBLE_REL (*d, 18.52, 0.001);

    YT_END();
}

YT_TEST (cargs, default_value_override)
{
    char* argv[] = { "dummy", "-A", "def", "-B", "23", "-C", "true", "-D", "220.72", NULL };
    int argc     = sizeof (argv) / sizeof (argv[0]);

    char* a   = cargs_add_arg ("A", "1st arg", String, "bcd");
    int* b    = cargs_add_arg ("B", "2nd arg", Integer, "13");
    bool* c   = cargs_add_arg ("C", "3rd arg", Boolean, "false");
    double* d = cargs_add_arg ("D", "4th arg", Double, "18.52");

    YT_EQ_SCALAR (true, cargs_parse_input (argc, argv));

    YT_EQ_STRING (a, "def");
    YT_EQ_SCALAR (*b, 23);
    YT_EQ_SCALAR (*c, true);
    YT_EQ_DOUBLE_REL (*d, 220.72, 0.001);

    YT_END();
}
YT_TEST (cargs, unknown_argument_in_cl)
{
    char* argv[] = { "dummy", "-B", "23", NULL };
    int argc     = sizeof (argv) / sizeof (argv[0]);

    cargs_add_arg ("A", "1st arg", String, NULL);

    YT_EQ_SCALAR (false, cargs_parse_input (argc, argv));
YT_TESTP (cargs, inactive_argument_in_cl, bool)
{
    bool is_inactive_arg_present_in_cl = YT_ARG_0();

    // "A" argument is only created to be used in the creation of the subarg, nothing else.
    // Note: Flag interface is used to keep test simpler.
    bool* a = cargs_add_arg ("A", "1st arg", Flag, "false");           // Default can also be true.
    cargs_add_subarg (a, always_false, "B", "2nd arg", Flag, "false"); // Default can also be true.

    if (is_inactive_arg_present_in_cl) {
        char* argv[] = { "dummy", "-B", NULL };
        YT_EQ_SCALAR (false, cargs_parse_input (ARRAY_LEN(argv), argv));
    } else {
        char* argv[] = { "dummy", NULL };
        YT_EQ_SCALAR (true, cargs_parse_input (ARRAY_LEN(argv), argv));
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
    // clang-format off
    required_arguments (4, YT_ARG (bool){ true, true, false, false },
                           YT_ARG (bool){ true, false, true, false });
    // clang-format on
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
