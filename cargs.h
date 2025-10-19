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

#ifdef CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE // null byte is not included
    #define CARGS_MAX_INPUT_VALUE_LEN CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE
#else
    #define CARGS_MAX_INPUT_VALUE_LEN 100 // null byte is not included
#endif                                    // CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE

#ifdef CARGS_ARGUMENT_PREFIX_CHAR_OVERRIDE
    #define CARGS__ARGUMENT_PREFIX_CHAR CARGS_ARGUMENT_PREFIX_CHAR_OVERRIDE
#else
    #define CARGS__ARGUMENT_PREFIX_CHAR '-'
#endif // CARGS_ARGUMENT_PREFIX_CHAR_OVERRIDE

#define CARGS_PARENT_OF(self, type, field) ((type*)((uintptr_t)self - offsetof (type, field)))

#define CARGS_UNUSED(v)                    (void)(v)

typedef struct {
    void* address;
    size_t len;
} Cargs_Slice;

typedef struct Cargs_TypeInterface {
    void* value; // When allow_multiple == true, value if of type Cargs_ArrayList. Rest of the
                 // fields will remain same as the Type the ArrayList will hold.
    char* name;
    char* format_help;
    size_t type_size;
    bool allow_multiple;
    bool is_flag; // set true for 'flag' arguments which don't need a value from command line
    bool (*parse_string) (struct Cargs_TypeInterface* self, const char* input, Cargs_Slice out);
} Cargs_TypeInterface;

typedef struct {
    void* buffer;
    size_t item_size;
    size_t capacity;
    size_t len;
} Cargs_ArrayList;

#define CARGS_ERROR(ret, msg, ...)                           \
    do {                                                     \
        fprintf (stderr, "ERROR: " msg "\n", ##__VA_ARGS__); \
        return ret;                                          \
    } while (0)

void cargs_panic (const char* msg);
void* cargs_add_subarg (void* parent_arg_value_addr, bool (*is_enabled_fn) (void), const char* name,
                        const char* description, Cargs_TypeInterface interface,
                        const char* default_value);
void* cargs_add_arg (const char* name, const char* description, Cargs_TypeInterface interface,
                     const char* default_value);
void cargs_cleanup();
bool cargs_parse_input (int argc, char** argv);
void cargs_print_help();

bool cargs_bool_parse_string (struct Cargs_TypeInterface* self, const char* input, Cargs_Slice out);
bool cargs_int_parse_string (struct Cargs_TypeInterface* self, const char* input, Cargs_Slice out);
bool cargs_string_parse_string (struct Cargs_TypeInterface* self, const char* input,
                                Cargs_Slice out);
bool cargs_flag_parse_string (struct Cargs_TypeInterface* self, const char* input, Cargs_Slice out);
bool cargs_double_parse_string (struct Cargs_TypeInterface* self, const char* input,
                                Cargs_Slice out);

void* cargs_arl_pop (Cargs_ArrayList* arl);

#define CARGS_LISTOF(ti)                                          \
    (assert (!ti.is_flag), (Cargs_TypeInterface){                 \
                               .name           = ti.name,         \
                               .format_help    = ti.format_help,  \
                               .type_size      = ti.type_size,    \
                               .is_flag        = false,           \
                               .allow_multiple = true,            \
                               .parse_string   = ti.parse_string, \
                           })

#ifdef CARGS_IMPLIMENTATION

typedef struct CARGS__Argument {
    char* name;
    char* description;
    char* default_value;
    bool provided; // true if some value was provided. Always true for optional arguments.
    bool dirty;    // true mean value was updated during parsing.
    Cargs_TypeInterface interface;
    struct {
        struct CARGS__Argument* parent_arg;
        bool (*is_enabled_fn) (void);
    } condition;
} CARGS__Argument;

unsigned int CARGS__parsing_done_count = 0;
unsigned int CARGS__arg_list_count     = 0;
CARGS__Argument* CARGS__arg_list[10];

void cargs_panic (const char* msg)
{
    if (msg != NULL) {
        fprintf (stderr, "CARGS__panic! %s\n", msg);
    }
    exit (1);
}

    #define CARGS__MIN(a, b) ((a) > (b) ? (b) : (a))
    #define CARGS__MAX(a, b) ((a) > (b) ? (a) : (b))

    #define CARGS__SLICE_OF(a, l)      \
        (Cargs_Slice)                  \
        {                              \
            .address = (a), .len = (l) \
        }

/*******************************************************************************************
 * ArrayList functions
 *********************************************************************************************/
Cargs_ArrayList* CARGS__arl_new_with_capacity (size_t capacity, size_t item_size)
{
    if (capacity == 0) {
        cargs_panic ("Array list must be provided non-zero capacity");
    }

    Cargs_ArrayList* newlist = (Cargs_ArrayList*)malloc (sizeof (Cargs_ArrayList));
    if (newlist == NULL) {
        perror ("ERROR: Allocation failed");
        cargs_panic (NULL);
    }

    newlist->capacity  = capacity;
    newlist->len       = 0;
    newlist->item_size = item_size;
    if (newlist->capacity > 0) {
        if (!(newlist->buffer = malloc (item_size * newlist->capacity))) {
            perror ("ERROR: Allocation failed");
            cargs_panic (NULL);
        }
    }

    return newlist;
}

void* CARGS__arl_append (Cargs_ArrayList* arl, void* c)
{
    if (arl->len >= arl->capacity) {
        assert (arl->capacity > 0);
        arl->capacity *= 2;
        arl->buffer = realloc (arl->buffer, arl->item_size * arl->capacity);
        if (arl->buffer == NULL) {
            perror ("ERROR: Relocation failed");
            cargs_panic (NULL);
        }
    }
    void* dest = (void*)((uintptr_t)arl->buffer + (arl->len * arl->item_size));
    if (c != NULL) {
        memcpy (dest, c, arl->item_size);
    }
    arl->len++;
    return dest;
}

void* cargs_arl_pop (Cargs_ArrayList* arl)
{
    if (arl->len > 0) {
        arl->len--;
        void* item = (void*)((uintptr_t)arl->buffer + (arl->len * arl->item_size));
        return item;
    }

    return NULL;
}

void* CARGS__arl_dealloc (Cargs_ArrayList* arl)
{
    assert (arl != NULL);
    free (arl->buffer);
    free (arl);
    return NULL;
}

/*******************************************************************************************
 * Argument functions
 *********************************************************************************************/
void CARGS__assign_value (Cargs_TypeInterface* interface, const char* input)
{
    // Special case for flags. Flag arguments must always have a default value, which gets acted on
    // when the flag argument is found during argument parsing.
    if (interface->is_flag) {
        assert (input != NULL);
        cargs_bool_parse_string (interface, input,
                                 CARGS__SLICE_OF (interface->value, interface->type_size));
    } else {
        if (input != NULL) {
            interface->parse_string (interface, input,
                                     CARGS__SLICE_OF (interface->value, interface->type_size));
        }
    }
}

CARGS__Argument* CARGS__find_by_value_address (const void* needle)
{
    for (unsigned i = 0; i < CARGS__arg_list_count; i++) {
        CARGS__Argument* arg = CARGS__arg_list[i];
        if (arg->interface.value == needle) {
            return arg;
        }
    }
    return NULL;
}

bool CARGS__is_arg_condition_enabled (CARGS__Argument* arg)
{
    if (arg->condition.parent_arg != NULL) {
        CARGS__Argument* parent_arg = arg->condition.parent_arg;
        if (parent_arg->interface.allow_multiple) {
            return ((Cargs_ArrayList*)parent_arg->interface.value)->len > 0;
        } else {
            return CARGS__is_arg_condition_enabled (parent_arg) && parent_arg->provided &&
                   arg->condition.is_enabled_fn();
        }
    }
    return true;
}

void* cargs_add_subarg (void* parent_arg_value_addr, bool (*is_enabled_fn) (void), const char* name,
                        const char* description, Cargs_TypeInterface interface,
                        const char* default_value)
{
    CARGS__Argument* parent_arg = CARGS__find_by_value_address (parent_arg_value_addr);
    assert (parent_arg != NULL);

    if (parent_arg->interface.allow_multiple && is_enabled_fn != NULL) {
        cargs_panic ("Sub command of List argument are automatically enabled/disabled.");
    }

    void* new_arg_value_addr = cargs_add_arg (name, description, interface, default_value);
    assert (new_arg_value_addr != NULL);

    CARGS__Argument* new_arg = CARGS__find_by_value_address (new_arg_value_addr);
    assert (new_arg != NULL);

    new_arg->condition.is_enabled_fn = is_enabled_fn;
    new_arg->condition.parent_arg    = parent_arg;

    return new_arg_value_addr;
}

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

    new_arg->dirty    = false; // Initially args are not dirty. Becomes dirty if was modified later.
    new_arg->provided = default_value != NULL;
    new_arg->interface               = interface;
    new_arg->condition.is_enabled_fn = NULL;
    new_arg->condition.parent_arg    = NULL;

    if (interface.allow_multiple) {
        if (default_value != NULL) {
            cargs_panic ("Lists do not have a default value");
        }
        new_arg->interface.value = CARGS__arl_new_with_capacity (10, interface.type_size);
    } else {
        if (!(new_arg->interface.value = malloc (new_arg->interface.type_size))) {
            perror ("ERROR: Allocation failed");
            cargs_panic (NULL);
        }

        CARGS__assign_value (&new_arg->interface, default_value);
    }

    CARGS__arg_list[CARGS__arg_list_count++] = new_arg;

    assert (new_arg != NULL); // Cannot be null since all previous errors must have been handled.
    return new_arg->interface.value;
}

void cargs_cleanup()
{
    for (unsigned i = 0; i < CARGS__arg_list_count; i++) {
        CARGS__Argument* arg = CARGS__arg_list[i];
        if (arg->interface.value != NULL) {
            if (arg->interface.allow_multiple) {
                CARGS__arl_dealloc ((Cargs_ArrayList*)arg->interface.value);
            } else {
                free (arg->interface.value);
            }
        }
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
        CARGS__Argument* arg = CARGS__arg_list[i];
        if (strncmp (arg->name, needle, CARGS__MAX_NAME_LEN) == 0) {
            return arg;
        }
    }
    return NULL;
}

bool cargs_parse_input (int argc, char** argv)
{
    CARGS__Argument* the_arg = NULL;

    CARGS_UNUSED (argc);
    argv++; // Skip first argument
    for (char* arg = NULL; (arg = *argv) != NULL; argv++) {
        // TODO: argument value/parameter might start with CARGS__ARGUMENT_PREFIX_CHAR

        if (arg[0] == CARGS__ARGUMENT_PREFIX_CHAR) {
            if (!(the_arg = CARGS__find_by_name (arg))) {
                CARGS_ERROR (false, "ERROR: Unknown argument '%s'", arg);
            }

            // Args updated during parsing are flaged dirty
            the_arg->dirty = true;

            // Flags do not have a value, so we have to call parse_string (which sets a calculated
            // value to the flag argument) now when it is first detected.
            if (the_arg->interface.is_flag) {
                the_arg->provided = the_arg->interface.parse_string (
                    &the_arg->interface, arg,
                    CARGS__SLICE_OF (the_arg->interface.value, the_arg->interface.type_size));
                // Special case for Help. If a help flag is found we skip the rest of the
                // parsing and simply return.
                if (strcmp ("help", the_arg->interface.name) == 0) {
                    goto exit;
                }
            }
        } else {
            assert (the_arg != NULL);

            if (the_arg->interface.allow_multiple) {
                void* new_item = CARGS__arl_append ((Cargs_ArrayList*)the_arg->interface.value,
                                                    NULL); // Dummy insert
                assert (new_item != NULL);

                the_arg->provided = the_arg->interface.parse_string (
                    &the_arg->interface, arg,
                    CARGS__SLICE_OF (new_item, the_arg->interface.type_size));
            } else {
                assert ((the_arg->default_value && the_arg->provided) ||
                        (!the_arg->default_value && !the_arg->provided));

                the_arg->provided = the_arg->interface.parse_string (
                    &the_arg->interface, arg,
                    CARGS__SLICE_OF (the_arg->interface.value, the_arg->interface.type_size));
            }
        }
    }

    for (unsigned i = 0; i < CARGS__arg_list_count; i++) {
        CARGS__Argument* the_arg = CARGS__arg_list[i];
        if (CARGS__is_arg_condition_enabled (the_arg)) {
            // Argument is enabled but not provided.
            if (!the_arg->provided) {
                CARGS_ERROR (false, "Argument '%s' is required but was not provided",
                             the_arg->name);
            }
        } else {
            // Argument is not enabled but was provided.
            if (the_arg->dirty) {
                CARGS_ERROR (false, "Argument '%s' has no effect", the_arg->name);
            }
        }
    }

exit:
    CARGS__parsing_done_count++; // Increment the number of times this parsing of argv was done.
    return true;
}

void CARGS__print_nested_args (CARGS__Argument* arg, int indent)
{
    fprintf (stderr, "%*s%-*s%-25s%s ", (indent + 1), " ", 15 - indent, arg->name,
             arg->interface.format_help, arg->description);
    if (arg->default_value) {
        fprintf (stderr, "(Default set to '%s')\n", arg->default_value);
    } else {
        fprintf (stderr, "(Required)\n");
    }

    for (unsigned i = 0; i < CARGS__arg_list_count; i++) {
        CARGS__Argument* the_arg = CARGS__arg_list[i];
        if (the_arg->condition.parent_arg != NULL && the_arg->condition.parent_arg == arg) {
            CARGS__print_nested_args (the_arg, indent + 2);
        }
    }
}

void cargs_print_help()
{
    printf ("Usage:\n");
    for (unsigned i = 0; i < CARGS__arg_list_count; i++) {
        int indent               = 1;
        CARGS__Argument* the_arg = CARGS__arg_list[i];
        if (the_arg->condition.parent_arg == NULL) {
            CARGS__print_nested_args (the_arg, indent);
        }
    }
}

/*******************************************************************************************
 * Interfaces
 *********************************************************************************************/

bool cargs_bool_parse_string (struct Cargs_TypeInterface* self, const char* input, Cargs_Slice out)
{
    assert (self != NULL);
    assert (self->value != NULL);
    assert (out.len == sizeof (bool));
    CARGS_UNUSED (self);

    if (strncmp ("true", input, CARGS_MAX_INPUT_VALUE_LEN) == 0) {
        *(bool*)out.address = true;
    } else if (strncmp ("false", input, CARGS_MAX_INPUT_VALUE_LEN) == 0) {
        *(bool*)out.address = false;
    } else {
        CARGS_ERROR (false, "Invalid boolean input");
    }

    return true;
}

bool cargs_int_parse_string (struct Cargs_TypeInterface* self, const char* input, Cargs_Slice out)
{
    assert (self != NULL);
    assert (self->value != NULL);
    assert (out.len == sizeof (int));
    CARGS_UNUSED (self);

    // TODO: strtol does not take length as an input. Bufffer overflow/security issue possible.

    errno              = 0; // To detect if strtol failed
    *(int*)out.address = strtol (input, NULL, 10);
    if (errno == ERANGE) {
        CARGS_ERROR (false, "Invalid integer input");
    }

    return true;
}

bool cargs_string_parse_string (struct Cargs_TypeInterface* self, const char* input,
                                Cargs_Slice out)
{
    assert (self != NULL);
    assert (self->value != NULL);
    CARGS_UNUSED (self);

    strncpy ((char*)out.address, input, CARGS__MIN (CARGS_MAX_INPUT_VALUE_LEN, out.len));
    return true;
}

bool cargs_flag_parse_string (struct Cargs_TypeInterface* self, const char* input, Cargs_Slice out)
{
    assert (self != NULL);
    assert (self->value != NULL);
    assert (out.len == sizeof (bool));
    CARGS_UNUSED (input);

    CARGS__Argument* the_arg = CARGS_PARENT_OF (self, CARGS__Argument, interface);
    assert (the_arg->default_value != NULL); // Flags must have a default value

    if (strncmp (the_arg->default_value, "false", CARGS_MAX_INPUT_VALUE_LEN) == 0) {
        *(bool*)out.address = true;
    } else if (strncmp (the_arg->default_value, "true", CARGS_MAX_INPUT_VALUE_LEN) == 0) {
        *(bool*)out.address = false;
    } else {
        cargs_panic ("Invalid boolean value");
    }

    return true;
}

bool cargs_double_parse_string (struct Cargs_TypeInterface* self, const char* input,
                                Cargs_Slice out)
{
    assert (self != NULL);
    assert (self->value != NULL);
    assert (out.len == sizeof (double));
    CARGS_UNUSED (self);

    // TODO: strtod does not take length as an input. Bufffer overflow/security issue possible

    errno = 0; // To detect if strtol failed

    *(double*)out.address = strtod (input, NULL);
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
    .type_size    = sizeof (bool),
    .parse_string = cargs_bool_parse_string,
};

Cargs_TypeInterface Integer = {
    .name         = "integer",
    .format_help  = "(number)",
    .type_size    = sizeof (int),
    .parse_string = cargs_int_parse_string,
};

Cargs_TypeInterface String = {
    .name         = "string",
    .format_help  = "(text)",
    .type_size    = sizeof (char) * CARGS_MAX_INPUT_VALUE_LEN,
    .parse_string = cargs_string_parse_string,
};

Cargs_TypeInterface Flag = {
    .name         = "flag",
    .format_help  = "",
    .type_size    = sizeof (bool),
    .is_flag      = true,
    .parse_string = cargs_flag_parse_string,
};

Cargs_TypeInterface Double = {
    .name         = "double",
    .format_help  = "(decimal number)",
    .type_size    = sizeof (double),
    .parse_string = cargs_double_parse_string,
};

Cargs_TypeInterface Help = {
    .name         = "help",
    .format_help  = "",
    .type_size    = sizeof (bool),
    .is_flag      = true,
    .parse_string = cargs_flag_parse_string,
};
