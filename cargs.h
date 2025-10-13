#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#define CARGS__MAX_NAME_LEN        50  // null byte is not included
#define CARGS__MAX_DESCRIPTION_LEN 100 // null byte is not included

#ifdef CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE
    #define CARGS_MAX_INPUT_VALUE_LEN \
        CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE // null byte is not included
#else
    #define CARGS_MAX_INPUT_VALUE_LEN 100 // null byte is not included

#endif // CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE

#ifdef CARGS_ARGUMENT_PREFIX_CHAR_OVERRIDE
    #define CARGS__ARGUMENT_PREFIX_CHAR CARGS_ARGUMENT_PREFIX_CHAR_OVERRIDE
#else
    #define CARGS__ARGUMENT_PREFIX_CHAR '-'
#endif // CARGS_ARGUMENT_PREFIX_CHAR_OVERRIDE

#define CARGS_PARENT_OF(self, type, field) ((type*)((uintptr_t)self - offsetof (type, field)))

#define CARGS_UNUSED(v)                    (void)(v)

typedef struct Cargs_TypeInterface {
    void* value;
    char* name;
    char* format_help;
    size_t type_size;
    void (*alloc) (struct Cargs_TypeInterface* self, const char* default_value);
    void (*free) (struct Cargs_TypeInterface* self);
    bool (*parse_string) (struct Cargs_TypeInterface* self, const char* input);
} Cargs_TypeInterface;

#define CARGS_ERROR(ret, msg, ...)                           \
    do {                                                     \
        fprintf (stderr, "ERROR: " msg "\n", ##__VA_ARGS__); \
        return ret;                                          \
    } while (0)

void cargs_panic (const char* msg);
void* cargs_add_arg (const char* name, const char* description, Cargs_TypeInterface interface,
                     const char* default_value);
void cargs_cleanup();
bool cargs_parse_input (int argc, char** argv);
void cargs_print_help();

void cargs_generic_free (struct Cargs_TypeInterface* self);
void cargs_default_alloc (struct Cargs_TypeInterface* self, const char* default_value);
bool cargs_bool_parse_string (struct Cargs_TypeInterface* self, const char* input);
bool cargs_int_parse_string (struct Cargs_TypeInterface* self, const char* input);
bool cargs_string_parse_string (struct Cargs_TypeInterface* self, const char* input);
bool cargs_flag_parse_string (struct Cargs_TypeInterface* self, const char* input);
bool cargs_double_parse_string (struct Cargs_TypeInterface* self, const char* input);

#ifdef CARGS_IMPLIMENTATION

typedef struct {
    char* name;
    char* description;
    char* default_value;
    bool provided; // true if some value was provided. Always true for optional arguments.
    Cargs_TypeInterface interface;
} CARGS__Argument;

unsigned int CARGS__parsing_done_count = 0;
unsigned int CARGS__arg_list_count     = 0;
CARGS__Argument* arg_list[10];

void cargs_panic (const char* msg)
{
    if (msg != NULL) {
        fprintf (stderr, "CARGS__panic! %s\n", msg);
    }
    exit (1);
}

/*******************************************************************************************
 * Argument functions
 *********************************************************************************************/
void* cargs_add_arg (const char* name, const char* description, Cargs_TypeInterface interface,
                     const char* default_value)
{
    CARGS__Argument* new_arg = NULL;

    if (CARGS__arg_list_count >= 10) {
        cargs_panic ("Too many arguments added");
    }

    if (!(new_arg = (CARGS__Argument*)malloc (sizeof (CARGS__Argument)))) {
        perror ("ERROR: Allocation failed");
        cargs_panic (NULL);
    }

    char temp_name[CARGS__MAX_NAME_LEN + 1] = { CARGS__ARGUMENT_PREFIX_CHAR,
                                                0 };    // +1 for '\0' char
    strncat (temp_name, name, CARGS__MAX_NAME_LEN - 1); // -1 because '-' char is already in

    if (!(new_arg->name = strndup (temp_name, CARGS__MAX_NAME_LEN))) {
        perror ("ERROR: Allocation failed");
        cargs_panic (NULL);
    }

    if (!(new_arg->description = strndup (description, CARGS__MAX_DESCRIPTION_LEN))) {
        perror ("ERROR: Allocation failed");
        cargs_panic (NULL);
    }

    if (default_value != NULL) {
        if (!(new_arg->default_value = strndup (default_value, CARGS__MAX_DESCRIPTION_LEN))) {
            perror ("ERROR: Allocation failed");
            cargs_panic (NULL);
        }
    } else {
        new_arg->default_value = NULL;
    }

    new_arg->provided  = default_value != NULL;
    new_arg->interface = interface;
    new_arg->interface.alloc (&new_arg->interface, default_value);

    arg_list[CARGS__arg_list_count++] = new_arg;

    return new_arg->interface.value;
}

void cargs_cleanup()
{
    for (unsigned i = 0; i < CARGS__arg_list_count; i++) {
        CARGS__Argument* arg = arg_list[i];
        arg->interface.free (&arg->interface);
        free (arg->name);
        free (arg->description);
        if (arg->default_value) {
            free (arg->default_value);
        }
        free (arg);
    }
}

CARGS__Argument* CARGS__find_by_name (const char* needle)
{
    for (unsigned i = 0; i < CARGS__arg_list_count; i++) {
        CARGS__Argument* arg = arg_list[i];
        if (strncmp (arg->name, needle, CARGS__MAX_NAME_LEN) == 0) {
            return arg;
        }
    }
    return NULL;
}

void CARGS__arguments_reset()
{
    // Clear provided field for non-optional arguments
    for (unsigned i = 0; i < CARGS__arg_list_count; i++) {
        CARGS__Argument* arg = arg_list[i];
        if (arg->default_value == NULL) {
            arg->provided = false;
        }
    }
}

bool cargs_parse_input (int argc, char** argv)
{
    CARGS__Argument* the_arg = NULL;
    bool state_is_key        = true;

    // Prepare for another parsing of args. Reset few states.
    if (CARGS__parsing_done_count > 0) {
        CARGS__arguments_reset();
    }

    CARGS_UNUSED (argc);
    argv++; // Skip first argument
    for (char* arg = NULL; (arg = *argv) != NULL; argv++) {
        if (state_is_key) {
            if (!(the_arg = CARGS__find_by_name (arg))) {
                continue;
            }
            state_is_key = false; // Now parse value for this key

            if (strcmp ("flag", the_arg->interface.name) == 0) {
                // Special case for flags. They are the only ones with just key and no value, so it
                // made little sense to add a 'is_flag' field in the Cargs_TypeInterface struct.
                the_arg->provided = the_arg->interface.parse_string (&the_arg->interface, arg);
                state_is_key      = true; // Now parse value for this key
            } else if (strcmp ("help", the_arg->interface.name) == 0) {
                // Special case for Help. If a help flag is found we skip the rest of the parsing
                // and simply return.
                the_arg->provided = the_arg->interface.parse_string (&the_arg->interface, arg);
                return true;
            }
        } else {
            assert (the_arg != NULL);
            assert ((the_arg->default_value && the_arg->provided) ||
                    (!the_arg->default_value && !the_arg->provided));

            the_arg->provided = the_arg->interface.parse_string (&the_arg->interface, arg);
            state_is_key      = true; // Now parse new key
        }
    }

    assert (state_is_key == true);

    for (unsigned i = 0; i < CARGS__arg_list_count; i++) {
        CARGS__Argument* the_arg = arg_list[i];
        if (!the_arg->provided) {
            CARGS_ERROR (false, "Argument '%s' is required but was not provided", the_arg->name);
        }
    }

    CARGS__parsing_done_count++; // Increment the number of times this parsing of argv was done.
    return true;
}

void cargs_print_help()
{
    printf ("Usage:\n");
    for (unsigned i = 0; i < CARGS__arg_list_count; i++) {
        CARGS__Argument* the_arg = arg_list[i];

        fprintf (stderr, "  %-10s\t%-20s %s ", the_arg->name, the_arg->interface.format_help,
                 the_arg->description);
        if (the_arg->default_value) {
            fprintf (stderr, "(Default set to '%s')\n", the_arg->default_value);
        } else {
            fprintf (stderr, "(Required)\n");
        }
    }
}

/*******************************************************************************************
 * Interfaces
 *********************************************************************************************/

void cargs_generic_free (struct Cargs_TypeInterface* self)
{
    if (self->value != NULL) {
        free (self->value);
    }
}

void cargs_default_alloc (struct Cargs_TypeInterface* self, const char* default_value)
{
    if (!(self->value = malloc (self->type_size))) {
        perror ("ERROR: Allocation failed");
        cargs_panic (NULL);
    }

    // Special case for flags ('help' is also a special flag). Flag arguments must always have a
    // default value, which gets later flipped when the flag argument is found during argument
    // parsing.
    if (strcmp ("flag", self->name) == 0 || strcmp ("help", self->name) == 0) {
        assert (default_value != NULL);
        cargs_bool_parse_string (self, default_value);
    } else {
        if (default_value != NULL) {
            self->parse_string (self, default_value);
        }
    }
}

bool cargs_bool_parse_string (struct Cargs_TypeInterface* self, const char* input)
{
    assert (self != NULL);
    assert (self->value != NULL);

    if (strncmp ("true", input, CARGS_MAX_INPUT_VALUE_LEN) == 0) {
        *(bool*)self->value = true;
    } else if (strncmp ("false", input, CARGS_MAX_INPUT_VALUE_LEN) == 0) {
        *(bool*)self->value = false;
    } else {
        CARGS_ERROR (false, "Invalid boolean input");
    }

    return true;
}

bool cargs_int_parse_string (struct Cargs_TypeInterface* self, const char* input)
{
    assert (self != NULL);
    assert (self->value != NULL);

    errno              = 0; // To detect if strtol failed
    *(int*)self->value = strtol (input, NULL, 10);
    if (errno == ERANGE) {
        CARGS_ERROR (false, "Invalid integer input");
    }

    return true;
}

bool cargs_string_parse_string (struct Cargs_TypeInterface* self, const char* input)
{
    assert (self != NULL);
    assert (self->value != NULL);

    strncpy ((char*)self->value, input, CARGS_MAX_INPUT_VALUE_LEN);
    return true;
}

bool cargs_flag_parse_string (struct Cargs_TypeInterface* self, const char* input)
{
    assert (self != NULL);
    assert (self->value != NULL);
    CARGS_UNUSED (input);

    CARGS__Argument* the_arg = CARGS_PARENT_OF (self, CARGS__Argument, interface);
    assert (the_arg->default_value != NULL); // Flags must have a default value

    if (strncmp (the_arg->default_value, "false", CARGS_MAX_INPUT_VALUE_LEN) == 0) {
        *(bool*)self->value = true;
    } else if (strncmp (the_arg->default_value, "true", CARGS_MAX_INPUT_VALUE_LEN) == 0) {
        *(bool*)self->value = false;
    } else {
        cargs_panic ("Invalid boolean value");
    }

    return true;
}

bool cargs_double_parse_string (struct Cargs_TypeInterface* self, const char* input)
{
    assert (self != NULL);
    assert (self->value != NULL);

    errno                 = 0; // To detect if strtol failed
    *(double*)self->value = strtod (input, NULL);
    if (errno == ERANGE) {
        CARGS_ERROR (false, "Invalid floating point input");
    }

    return true;
}
#endif // CARGS_IMPLIMENTATION

/*******************************************************************************************
 * Interface types
 *********************************************************************************************/

Cargs_TypeInterface Boolean = {
    .name         = "boolean",
    .format_help  = "(false|true)",
    .type_size    = sizeof (int),
    .alloc        = cargs_default_alloc,
    .free         = cargs_generic_free,
    .parse_string = cargs_bool_parse_string,
};

Cargs_TypeInterface Integer = {
    .name         = "integer",
    .format_help  = "(number)",
    .type_size    = sizeof (int),
    .alloc        = cargs_default_alloc,
    .free         = cargs_generic_free,
    .parse_string = cargs_int_parse_string,
};

Cargs_TypeInterface String = {
    .name         = "string",
    .format_help  = "(text)",
    .type_size    = sizeof (char) * CARGS_MAX_INPUT_VALUE_LEN,
    .alloc        = cargs_default_alloc,
    .free         = cargs_generic_free,
    .parse_string = cargs_string_parse_string,
};

Cargs_TypeInterface Flag = {
    .name         = "flag",
    .format_help  = "",
    .type_size    = sizeof (int),
    .alloc        = cargs_default_alloc,
    .free         = cargs_generic_free,
    .parse_string = cargs_flag_parse_string,
};

Cargs_TypeInterface Double = {
    .name         = "double",
    .format_help  = "(decimal number)",
    .type_size    = sizeof (double),
    .alloc        = cargs_default_alloc,
    .free         = cargs_generic_free,
    .parse_string = cargs_double_parse_string,
};

Cargs_TypeInterface Help = {
    .name         = "help",
    .format_help  = "",
    .type_size    = sizeof (int),
    .alloc        = cargs_default_alloc,
    .free         = cargs_generic_free,
    .parse_string = cargs_flag_parse_string,
};
