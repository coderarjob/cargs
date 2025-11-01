#include <stdio.h>
#define CARGS_IMPLEMENTATION
#include "../cargs.h"

#define USAGE(p)                                                                         \
    do {                                                                                 \
        fprintf (stderr, "Tiny Encryption Algorithm implementation, with 128 bit key.\n" \
                         "Performs Encryption/Decruption of multiple flies.\n");         \
        cargs_print_help();                                                              \
    } while (0)

typedef struct {
    bool* do_encrypt;
    bool* output_to_stdout;
    bool* post_delete_enabled;
    bool* verbose_enabled;
    Cargs_ArrayList* infiles;
    bool* key_from_stdin;
    char* key;
    bool* display_help;
} Config;

Config config = { 0 };

static bool stdout_print_enable (void)
{
    return *config.do_encrypt == false;
}

static bool key_as_param_enable (void)
{
    return *config.key_from_stdin == false;
}

static bool post_delete_enable (void)
{
    return *config.output_to_stdout == false;
}

static bool verbose_is_enabled (void)
{
    return *config.output_to_stdout == false;
}

int main (int argc, char** argv)
{
    config.do_encrypt = cargs_add_arg ("e", "Encrypt files, decrypt otherwise", Flag, "false");

    config.key_from_stdin = cargs_add_arg ("K", "16 byte key (from stdin)", Flag, "false");

    config.infiles = cargs_add_arg ("I", "Files that need to be processed", CARGS_LISTOF (String),
                                    NULL);

    config.output_to_stdout = cargs_add_cond_arg_d (stdout_print_enable, "When -e is false", "N",
                                                    "Display output to stdout", Flag, "false");

    config.post_delete_enabled =
        cargs_add_cond_arg_d (post_delete_enable, "When -N is false", "D",
                              "Deletes input files after encryption/decryption", Flag, "false");

    config.verbose_enabled = cargs_add_cond_arg_d (verbose_is_enabled, "When -N is false", "v",
                                                   "Verbose", Flag, "false");

    config.display_help = cargs_add_arg ("h", "Display this help message", Help, "false");

    config.key = cargs_add_cond_arg_d (key_as_param_enable, "When -K is false", "k",
                                       "16 byte key (as argument)", String, NULL);

    if (!cargs_parse_input (argc, argv)) {
        USAGE (argv[0]);
        return 1;
    }

    if (*config.display_help) {
        USAGE (argv[0]);
        return 0;
    }

    printf ("Mode: %s\n", (*config.do_encrypt) ? "Encrypt" : "Decrypt");
    printf ("Post deletion: %d\n", *config.post_delete_enabled);
    printf ("Verbose: %d\n", *config.verbose_enabled);
    printf ("Key from (stdin): %d\n", *config.key_from_stdin);
    if (*config.key_from_stdin == false) {
        printf ("Key provided (as parameter): %s\n", config.key);
    }
    if (*config.do_encrypt == false) {
        printf ("Decrypt and display: %d\n", *config.output_to_stdout);
    }
    printf ("Number of input files: %ld\n", config.infiles->len);

    for (unsigned i = 0; i < config.infiles->len; i++) {
        printf ("* %s\n", ((Cargs_StringType*)config.infiles->buffer)[i]);
    }

    cargs_cleanup();
    return 0;
}
