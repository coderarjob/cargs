#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

/*
 * Idea:
 *
 * char* name = cargs_arg(STRING, "out", "Output file path", NULL);
 * bool can_override = cargs_arg(BOOL, "override", "Overrides output file if it
 * exists", false);
 *
 * if (!cargs_parse(argc, argv)) {
 *  cargs_print_help();
 *  exit(1);
 * }
 */

typedef enum {
    ARG_TYPE_BOOL,
    ARG_TYPE_STRING,
    ARG_TYPE_INTEGER
} ArgumentTypes;

typedef struct {
    char* name;
    char* description;
    bool is_optional;
    bool provided; // true if some value was provided. Always true for optional arguments.
    ArgumentTypes type;
    union {
        char* string;
        bool boolean;
        int integer;
    } value;
} Argument;

#define MAX_NAME_LEN        50  // null byte is not included
#define MAX_DESCRIPTION_LEN 500 // null byte is not included
#define MAX_STRING_ARG_LEN  500 // null byte is not included

unsigned int arg_list_count = 0;
Argument* arg_list[10];

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

Argument* argument_alloc (const char* name, const char* description, ArgumentTypes type,
                          bool is_optional, va_list default_value, void** arg_value)
{
    Argument* new = NULL;
    if (!(new = malloc (sizeof (Argument)))) {
        perror ("ERROR: Allocation failed");
        return NULL;
    }

    char temp_name[MAX_NAME_LEN + 1] = { '-', 0 }; // +1 for '\0' char
    strncat (temp_name, name, MAX_NAME_LEN - 1);   // -1 because '-' char is already in

    if (!(new->name = strndup (temp_name, MAX_NAME_LEN))) {
        perror ("ERROR: Allocation failed");
        return NULL;
    }

    if (!(new->description = strndup (description, MAX_DESCRIPTION_LEN))) {
        perror ("ERROR: Allocation failed");
        return NULL;
    }

    new->type        = type;
    new->is_optional = is_optional;
    new->provided    = is_optional;

    // Default value for optional arguments
    switch (type) {
    case ARG_TYPE_BOOL:
        if (is_optional) {
            new->value.boolean = va_arg (default_value, int);
        }
        *arg_value = &new->value.boolean;
        break;
    case ARG_TYPE_INTEGER:
        if (is_optional) {
            new->value.integer = va_arg (default_value, int);
        }
        *arg_value = &new->value.integer;
        break;
    case ARG_TYPE_STRING: {
        new->value.string = NULL; // Default
        if (is_optional) {
            char* input = va_arg (default_value, char*);
            if (input != NULL) {
                if (!(new->value.string = strndup (input, MAX_STRING_ARG_LEN))) {
                    perror ("ERROR: Allocation failed");
                    return NULL;
                }
            }
        }
        assert (new->value.string != NULL);
        *arg_value = &new->value.string;
    } break;
    default:
        assert (false);
    }

    return new;
}

void argument_free (Argument* arg)
{
    if (arg->type == ARG_TYPE_STRING && arg->value.string != NULL) {
        free (arg->value.string);
    }

    free (arg->name);
    free (arg->description);
    free (arg);
}

void* argument_add (const char* name, const char* description, ArgumentTypes type, bool is_optional,
                    ...)
{
    void* ret = NULL;

    if (arg_list_count >= 10) {
        return NULL; // No space left
    }

    va_list l;
    va_start (l, is_optional);

    if (!(arg_list[arg_list_count++] = argument_alloc (name, description, type, is_optional, l,
                                                       &ret))) {
        // May be we should just panic on failure.
        return NULL;
    }
    va_end (l);

    return ret;
}

Argument* find_by_name (const char* needle)
{
    for (unsigned i = 0; i < arg_list_count; i++) {
        Argument* this = arg_list[i];
        if (strncmp (this->name, needle, MAX_NAME_LEN) == 0) {
            return this;
        }
    }
    return NULL;
}

bool argument_parse (int argc, char** argv)
{
    Argument* this    = NULL;
    bool state_is_key = true;

    (void)argc;
    argv++; // Skip first argument
    for (char* arg = NULL; (arg = *argv) != NULL; argv++) {
        if (state_is_key) {
            if (!(this = find_by_name (arg))) {
                fprintf (stderr, "ERROR: Argument '%s' is unknown\n", arg);
                return false;
            }
            state_is_key = false; // Now parse value for this key
        } else {
            assert (this != NULL);
            assert ((this->is_optional && this->provided) ||
                    (!this->is_optional && !this->provided));

            this->provided = true;

            switch (this->type) {
            case ARG_TYPE_BOOL: {
                this->value.boolean = strncmp ("1", arg, MAX_STRING_ARG_LEN) == 0;
            } break;
            case ARG_TYPE_INTEGER: {
                errno               = 0; // To detect is call to strtol failed
                this->value.integer = strtol (arg, NULL, 10);
                if (errno == ERANGE) {
                    perror ("ERROR: Invalid number");
                    return false;
                }
            } break;
            case ARG_TYPE_STRING: {
                char* value = strndup (arg, MAX_STRING_ARG_LEN);
                if (this->value.string != NULL) {
                    free (this->value.string); // Free default value
                }
                this->value.string = value;
            } break;
            default:
                assert (false);
                break;
            }
            state_is_key = true; // Now parse new key
        }
    }

    assert (state_is_key == true);

    for (unsigned i = 0; i < arg_list_count; i++) {
        Argument* this = arg_list[i];
        if (!this->provided) {
            fprintf (stderr, "Argument '%s' is required but was not provided\n", this->name);
            return false;
        }
    }
    return true;
}

void print_help()
{
    printf ("USAGE:\n");
    for (unsigned i = 0; i < arg_list_count; i++) {
        Argument* this = arg_list[i];
        printf ("  %-20s\t%s (%s)\n", this->name, this->description,
                (this->is_optional) ? "TODO" : "Required");
    }
}

void print_arugment (Argument* a)
{
    printf ("Argument:\n name: %s\n description: %s\n type: %d\n value: %s\n", a->name,
            a->description, a->type, a->value.string);
}

int main (int argc, char** argv)
{
    char** outfile = argument_add ("out", "Output file path", ARG_TYPE_STRING, true, "test.ppm");
    bool* override = argument_add ("override", "Overrides Output file if it exists", ARG_TYPE_BOOL,
                                   false);
    int* gain      = argument_add ("gain", "Gain of the amplifier", ARG_TYPE_INTEGER, 0);

    if (!argument_parse (argc, argv)) {
        print_help();
        return 1;
    }

    printf ("outfile: %s\n", *outfile);
    printf ("override: %d\n", *override);
    printf ("gain: %d\n", *gain);
    return 0;
}
