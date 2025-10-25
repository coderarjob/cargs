/*
 * CArgs is command line argument parsing single header library
 * Copyright (c) 2025 Arjob Mukherjee
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ----------------------------------------------------------------------------
 */

#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef CARGS_MAX_ARG_COUNT_OVERRIDE // This many arguments are allowed
    #define CARGS__MAX_ARG_COUNT CARGS_MAX_ARG_COUNT_OVERRIDE
#else
    #define CARGS__MAX_ARG_COUNT 20 // This many arguments are allowed
#endif                              // CARGS_MAX_ARG_COUNT_OVERRIDE

#ifdef CARGS_MAX_NAME_LEN_OVERRIDE // null byte is not included
    #define CARGS__MAX_NAME_LEN CARGS_MAX_NAME_LEN_OVERRIDE
#else
    #define CARGS__MAX_NAME_LEN 50 // null byte is not included
#endif                             // CARGS_MAX_NAME_LEN_OVERRIDE

#ifdef CARGS_MAX_DESCRIPTION_LEN_OVERRIDE // null byte is not included
    #define CARGS__MAX_DESCRIPTION_LEN CARGS_MAX_DESCRIPTION_LEN_OVERRIDE
#else
    #define CARGS__MAX_DESCRIPTION_LEN 100 // null byte is not included
#endif                                     // CARGS_MAX_DESCRIPTION_LEN_OVERRIDE

#ifdef CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE // null byte is not included
    #define CARGS_MAX_INPUT_VALUE_LEN CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE
#else
    #define CARGS_MAX_INPUT_VALUE_LEN 100 // null byte is not included
#endif                                    // CARGS_MAX_INPUT_VALUE_LEN_OVERRIDE

#ifdef CARGS_ARGUMENT_PREFIX_CHAR_OVERRIDE
    #define CARGS__ARGUMENT_PREFIX_CHAR CARGS_ARGUMENT_PREFIX_CHAR_OVERRIDE
#else
    #define CARGS__ARGUMENT_PREFIX_CHAR "-"
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

void* CARGS__cargs_add_arg (const char* name, const char* description,
                            Cargs_TypeInterface interface, const char* default_value,
                            bool (*is_enabled_fn) (void));

#define CARGS__call_cargs_add_arg(name, description, interface, default_value, is_enabled_fn)     \
    ({                                                                                            \
        static_assert (name != NULL, "must be string literal");                                   \
        static_assert (description != NULL, "must be string literal");                            \
        static_assert (default_value == NULL || default_value != NULL, "must be string literal"); \
        CARGS__cargs_add_arg (CARGS__ARGUMENT_PREFIX_CHAR name, description, interface,           \
                              default_value, is_enabled_fn);                                      \
    })

#define cargs_add_arg(name, description, interface, default_value) \
    CARGS__call_cargs_add_arg (name, description, interface, default_value, NULL)

#define cargs_add_cond_arg(is_enabled_fn, name, description, interface, default_value) \
    CARGS__call_cargs_add_arg (name, description, interface, default_value, is_enabled_fn)

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

#ifdef CARGS_IMPLEMENTATION

typedef struct CARGS__Argument {
    char* name;
    char* description;
    char* default_value;
    bool provided; // true if some value was provided. Always true for optional arguments.
    bool dirty;    // true mean value was updated during parsing.
    Cargs_TypeInterface interface;
    bool (*is_enabled_fn) (void); // If NULL, arg is always enabled, otherwise its enabled when this
                                  // predicate returns true.
} CARGS__Argument;

unsigned int CARGS__arg_list_count = 0;
CARGS__Argument* CARGS__arg_list[CARGS__MAX_ARG_COUNT];

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

    #define CARGS__ARRAY_LEN(a) (sizeof (a) / sizeof (a[0]))

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

bool CARGS__is_arg_enabled (CARGS__Argument* arg)
{
    if (arg->is_enabled_fn != NULL) {
        return arg->is_enabled_fn();
    }
    return true;
}

void* CARGS__cargs_add_arg (const char* name, const char* description,
                            Cargs_TypeInterface interface, const char* default_value,
                            bool (*is_enabled_fn) (void))
{
    CARGS__Argument* new_arg = NULL;

    if (CARGS__arg_list_count >= CARGS__ARRAY_LEN (CARGS__arg_list)) {
        cargs_panic ("Too many arguments added");
    }

    if (!(new_arg = (CARGS__Argument*)malloc (sizeof (CARGS__Argument)))) {
        perror ("ERROR: Allocation failed");
        cargs_panic (NULL);
    }

    new_arg->name          = (char*)name;
    new_arg->description   = (char*)description;
    new_arg->default_value = (char*)default_value;
    new_arg->dirty    = false; // Initially args are not dirty. Becomes dirty if was modified later.
    new_arg->provided = default_value != NULL;
    new_arg->interface     = interface;
    new_arg->is_enabled_fn = is_enabled_fn;

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
        free (arg);
    }
    CARGS__arg_list_count = 0;
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

        if (arg[0] == CARGS__ARGUMENT_PREFIX_CHAR[0]) {
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
        if (CARGS__is_arg_enabled (the_arg)) {
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
    return true;
}

static void CARGS__print_help_message (CARGS__Argument* arg, size_t max_arg_name_len,
                                       size_t max_arg_format_help_len)
{
    fprintf (stderr, "%-*s %-*s %s ", (int)max_arg_name_len, arg->name,
             (int)max_arg_format_help_len, arg->interface.format_help, arg->description);

    if (arg->default_value) {
        fprintf (stderr, "(Default set to '%s')\n", arg->default_value);
    } else {
        fprintf (stderr, "(Required)\n");
    }
}

void cargs_print_help()
{
    size_t max_arg_name_len        = 0;
    size_t max_arg_format_help_len = 0;

    for (unsigned i = 0; i < CARGS__arg_list_count; i++) {
        CARGS__Argument* the_arg = CARGS__arg_list[i];
        max_arg_name_len         = CARGS__MAX (strlen (the_arg->name), max_arg_name_len);
        max_arg_format_help_len  = CARGS__MAX (strlen (the_arg->interface.format_help),
                                               max_arg_format_help_len);
    }

    assert (max_arg_name_len > 0 && max_arg_name_len <= CARGS_MAX_INPUT_VALUE_LEN);
    assert (max_arg_format_help_len > 0 && max_arg_format_help_len <= CARGS_MAX_INPUT_VALUE_LEN);

    size_t conditional_arg_count = 0;

    fprintf (stderr, "Usage:\n");
    for (unsigned i = 0; i < CARGS__arg_list_count; i++) {
        CARGS__Argument* the_arg = CARGS__arg_list[i];
        if (the_arg->is_enabled_fn == NULL) {
            CARGS__print_help_message (the_arg, max_arg_name_len, max_arg_format_help_len);
        } else {
            conditional_arg_count++;
        }
    }

    if (conditional_arg_count == 0) {
        return; // No conditional arguments
    }

    fprintf (stderr, "Conditional:\n");
    for (unsigned i = 0; i < CARGS__arg_list_count; i++) {
        CARGS__Argument* the_arg = CARGS__arg_list[i];
        if (the_arg->is_enabled_fn != NULL) {
            CARGS__print_help_message (the_arg, max_arg_name_len, max_arg_format_help_len);
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

    assert (out.len - 1 <= CARGS_MAX_INPUT_VALUE_LEN);

    // Copies at most CARGS_MAX_INPUT_VALUE_LEN number of non-null characters.
    char* last = stpncpy ((char*)out.address, input, CARGS_MAX_INPUT_VALUE_LEN);

    // If input has more than CARGS_MAX_INPUT_VALUE_LEN chars, then stpncpy copies that many chars
    // to the destination but does not add null termination byte. This is why the below line is
    // important.
    *last = '\0';

    // Assert: Last character does not exceed the allocated/out buffer length.
    assert (((uintptr_t)last - (uintptr_t)out.address + 1) <= out.len);

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
#endif // CARGS_IMPLEMENTATION

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
    .name        = "string",
    .format_help = "(text)",
    .type_size   = sizeof (char) * CARGS_MAX_INPUT_VALUE_LEN + 1, // +1 for the NULL byte which the
                                                                  // CARGS_MAX_INPUT_VALUE_LEN does
                                                                  // not include
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
