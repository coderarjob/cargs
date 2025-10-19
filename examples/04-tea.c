#include <stdio.h>
#define CARGS_IMPLIMENTATION
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
    config.do_encrypt     = cargs_add_arg ("e", "Encrypt files, decrypt otherwise", Flag, "false");
    config.key_from_stdin = cargs_add_arg ("K", "16 byte key (from stdin)", Flag, "false");
    config.infiles = cargs_add_arg ("I", "Files that need to be processed", CARGS_LISTOF (String),
                                    NULL);
    config.output_to_stdout = cargs_add_subarg (config.do_encrypt, stdout_print_enable, "N",
                                                "When decrypting, display output to stdout", Flag,
                                                "false");
    config
        .post_delete_enabled = cargs_add_subarg (config.output_to_stdout, post_delete_enable, "D",
                                                 "Deletes input files after encryption/decryption",
                                                 Flag, "false");
    config.verbose_enabled   = cargs_add_subarg (config.output_to_stdout, verbose_is_enabled, "v",
                                                 "Verbose", Flag, "false");
    config.display_help      = cargs_add_arg ("h", "Display this help message", Flag, "false");

    config.key = cargs_add_subarg (config.key_from_stdin, key_as_param_enable, "k",
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
    for (char* file = cargs_arl_pop (config.infiles); file != NULL;
         file       = cargs_arl_pop (config.infiles)) {
        printf ("* %s\n", file);
    }

    cargs_cleanup();
    return 0;
}
