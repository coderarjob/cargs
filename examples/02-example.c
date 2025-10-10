#include <stdio.h>

#define CARGS_IMPLIMENTATION
#include "../cargs.h"

typedef enum {
    MODE_SINE_WAVE = 0,
    MODE_AM_WAVE   = 1,
    MODE_NOISE     = 2,
    MODES_COUNT
} Modes;

bool modes_parse_string (struct Cargs_TypeInterface* self, const char* input)
{
    assert (self != NULL);

    Modes mode = 0;
    if (strncmp ("sine", input, CARGS_MAX_INPUT_VALUE_LEN) == 0) {
        mode = MODE_SINE_WAVE;
    } else if (strncmp ("am", input, CARGS_MAX_INPUT_VALUE_LEN) == 0) {
        mode = MODE_AM_WAVE;
    } else if (strncmp ("noise", input, CARGS_MAX_INPUT_VALUE_LEN) == 0) {
        mode = MODE_NOISE;
    } else {
        CARGS_ERROR (false, "Invalid mode: '%s'", input);
    }

    *(Modes*)self->value = mode;
    return true;
}

Cargs_TypeInterface ModesInterface = {
    .name         = "Modes",
    .format_help  = "(sine|am|noise)",
    .alloc        = cargs_default_alloc,
    .type_size    = sizeof (Modes),
    .free         = cargs_generic_free,
    .parse_string = modes_parse_string,
};

int main (int argc, char** argv)
{
    Modes* mode = cargs_add_arg ("mode", "Mode of wave generation", ModesInterface, NULL);

    if (!cargs_parse_input (argc, argv)) {
        cargs_print_help();
        return 1;
    }

    double* freq = NULL;
    if (*mode == MODE_AM_WAVE || *mode == MODE_SINE_WAVE) {
        freq = cargs_add_arg ("freq", "Wave frequency", Double, NULL);

        if (!cargs_parse_input (argc, argv)) {
            cargs_print_help();
            return 1;
        }
    }

    printf ("mode: %d\n", *mode);
    if (*mode == MODE_AM_WAVE || *mode == MODE_SINE_WAVE) {
        printf ("freq: %f\n", *freq);
    }

    cargs_cleanup();
    return 0;
}
