#include <stdbool.h>
#include <stdio.h>
#define CARGS_IMPLIMENTATION
#define CARGS_ARGUMENT_PREFIX_CHAR_OVERRIDE '/'
#include "../cargs.h"

#define USAGE()                                \
    do {                                       \
        fprintf (stderr, "Example program\n"); \
        cargs_print_help();                    \
    } while (0)

typedef enum {
    UNSET,
    ENCRYPT,
    DECRYPT
} OpMode;

int main (int argc, char** argv)
{
    bool* encrypt          = cargs_add_arg ("e", "Encrypt", Boolean, NULL);
    char* key              = cargs_add_arg ("k", "16 byte key (as argument)", String, NULL);
    Cargs_ArrayList* files = cargs_add_arg ("I", "Input files to be processed", LISTOF (String),
                                            NULL);
    bool* show_help        = cargs_add_arg ("h", "Show usage", Help, "false");

    if (!cargs_parse_input (argc, argv)) {
        USAGE();
        return 1;
    }

    if (*show_help) {
        USAGE();
        return 0;
    }

    printf ("encrypt: %d\n", *encrypt);
    printf ("key: %s\n", key);
    printf ("Number of files: %ld\n", files->len);
    for (char* file = (char*)cargs_arl_pop (files); file != NULL;
         file       = (char*)cargs_arl_pop (files)) {
        printf ("Files: %s\n", file);
    }
    cargs_cleanup();
    return 0;
}
