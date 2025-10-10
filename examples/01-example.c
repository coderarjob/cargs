#include <stdio.h>

#define CARGS_IMPLIMENTATION
#include "../cargs.h"

int main (int argc, char** argv)
{
    char* outfile  = cargs_add_arg ("out", "Output file path", String, "test.ppm");
    bool* override = cargs_add_arg ("override", "Overrides Output file if it exists", Boolean,
                                    NULL);
    int* gain      = cargs_add_arg ("gain", "Gain of the amplifier", Integer, "0");
    bool* root     = cargs_add_arg ("R", "Run the application as root", Flag, "false");
    double* offset = cargs_add_arg ("offset", "Offset for the amplifer", Double, "13.6");

    if (!cargs_parse_input (argc, argv)) {
        cargs_print_help();
        return 1;
    }

    printf ("outfile: %s\n", outfile);
    printf ("override: %d\n", *override);
    printf ("gain: %d\n", *gain);
    printf ("root: %d\n", *root);
    printf ("offset: %f\n", *offset);

    cargs_cleanup();
    return 0;
}
