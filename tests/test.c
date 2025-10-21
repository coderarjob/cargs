#define YUKTI_TEST_IMPLEMENTATION
#include "yukti.h"

#define CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE 10 // A small enough number for easy testing
#define CARGS_IMPLEMENTATION
#include "../cargs.h"

YT_TEST (cargs, required_arguments_success)
{
    char* argv[] = { "dummy", "-A", "abc", "-B", "1", "-C", "true", "-D", "12.84", NULL };
    int argc     = sizeof (argv) / sizeof (argv[0]);

    char* a   = cargs_add_arg ("A", "1st arg", String, NULL);
    int* b    = cargs_add_arg ("B", "2nd arg", Integer, NULL);
    bool* c   = cargs_add_arg ("C", "3rd arg", Boolean, NULL);
    double* d = cargs_add_arg ("D", "4th arg", Double, NULL);

    YT_EQ_SCALAR (true, cargs_parse_input (argc, argv));

    YT_EQ_STRING (a, "abc");
    YT_EQ_SCALAR (*b, 1);
    YT_EQ_SCALAR (*c, true);
    YT_EQ_DOUBLE_REL (*d, 12.84, 0.001);

    YT_END();
}

YT_TEST (cargs, argument_value_length_clamping)
{
    char* argv[] = { "dummy", "-A", "123456789ABCD", NULL };
    int argc     = sizeof (argv) / sizeof (argv[0]);

    char* a = cargs_add_arg ("A", "1st arg", String, NULL);
    YT_EQ_SCALAR (true, cargs_parse_input (argc, argv));

    // CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE number of non-null characters
    YT_EQ_STRING (a, "123456789A");
    YT_END();
}

YT_TEST (cargs, required_arguments_not_met)
{
    char* argv[] = { "dummy", "-A", "abc", "-C", "true", "-D", "12.84", NULL };
    int argc     = sizeof (argv) / sizeof (argv[0]);

    cargs_add_arg ("A", "1st arg", String, NULL);
    cargs_add_arg ("B", "2nd arg", Integer, NULL);
    cargs_add_arg ("C", "3rd arg", Boolean, NULL);
    cargs_add_arg ("D", "4th arg", Double, NULL);

    YT_EQ_SCALAR (false, cargs_parse_input (argc, argv));

    YT_END();
}

YT_TEST (cargs, help_argument_present)
{
    // When help flag is found during parsing, the check to see if all required arguments were
    // provided is not done, thus this test is same as `required_arguments_not_met` but with one
    // additional Help arg and as a result cargs_parse_input would pass this time.

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

void yt_reset (void)
{
    cargs_cleanup();
}

int main (void)
{
    YT_INIT();
    required_arguments_success();
    required_arguments_not_met();
    default_values();
    default_value_override();
    help_argument_present();
    argument_value_length_clamping();
    YT_RETURN_WITH_REPORT();
}
