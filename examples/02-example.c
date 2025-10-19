#include <stdbool.h>
#include <stdio.h>

#define CARGS_IMPLIMENTATION
#include "../cargs.h"

typedef enum {
    MODE_SINE_WAVE = 0,
    MODE_AM_WAVE   = 1,
    MODE_NOISE     = 2,
    MODES_COUNT
} Modes;

bool modes_parse_string (struct Cargs_TypeInterface* self, const char* input, Cargs_Slice out)
{
#ifdef NDEBUG
    assert (self != NULL);
    assert (out.len == sizeof (Modes));
#else
    CARGS_UNUSED (self);
#endif // NDEBUG

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

    *(Modes*)out.address = mode;
    return true;
}

Cargs_TypeInterface ModesInterface = {
    .name         = "Modes",
    .format_help  = "(sine|am|noise)",
    .type_size    = sizeof (Modes),
    .parse_string = modes_parse_string,
};

typedef struct {
    Modes* do_encrypt;
    bool* show_help;
    double* freq;
    bool* interpolate;
} Config;

Config config = { 0 };

static bool freq_is_enabled (void)
{
    return (*config.do_encrypt == MODE_AM_WAVE || *config.do_encrypt == MODE_SINE_WAVE);
}

int main (int argc, char** argv)
{
    config.do_encrypt = cargs_add_arg ("mode", "Mode of wave generation", ModesInterface, NULL);
    config.show_help  = cargs_add_arg ("h", "Show usage", Help, "false");
    config.freq       = cargs_add_subarg (config.do_encrypt, freq_is_enabled, "freq",
                                          "Sine/AM wave frequency", Double, NULL);

    if (!cargs_parse_input (argc, argv)) {
        cargs_print_help();
        return 1;
    }

    if (*config.show_help) {
        cargs_print_help();
        return 0;
    }

    printf ("mode: %d\n", *config.do_encrypt);
    printf ("freq: %f\n", *config.freq);

    cargs_cleanup();
    return 0;
}
