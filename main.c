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

#define MAX_NAME_LEN        50  // null byte is not included
#define MAX_DESCRIPTION_LEN 500 // null byte is not included
#define MAX_STRING_ARG_LEN  500 // null byte is not included

#define MIN(a, b)           (((a) < (b)) ? (a) : (b))

typedef struct TypeInterface {
    void* value;
    char* (*name)();
    void (*alloc) (struct TypeInterface* self, va_list default_value);
    void (*free) (struct TypeInterface* self);
    bool (*parse_string) (struct TypeInterface* self, const char* input);
    void (*to_string) (struct TypeInterface* self, char* out, size_t size);
    char* (*format_help)();
} TypeInterface;

typedef struct {
    char* name;
    char* description;
    bool is_optional;
    bool provided; // true if some value was provided. Always true for optional arguments.
    TypeInterface* interface;
} Argument;
/*******************************************************************************************
 * Argument functions
 *********************************************************************************************/
unsigned int arg_list_count = 0;
Argument* arg_list[10];

void* argument_add (const char* name, const char* description, TypeInterface* interface,
                    bool is_optional, ...)
{
    Argument* new = NULL;

    if (arg_list_count >= 10) {
        return NULL;
    }

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

    new->is_optional = is_optional;
    new->provided    = is_optional;
    new->interface   = interface;

    va_list l;
    va_start (l, is_optional);
    new->interface->alloc (new->interface, l);
    va_end (l);

    arg_list[arg_list_count++] = new;

    return new->interface->value;
}

void argument_free (Argument* arg)
{
    arg->interface->free (arg->interface);
    free (arg->name);
    free (arg->description);
    free (arg);
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

            this->provided = this->interface->parse_string (this->interface, arg);
            state_is_key   = true; // Now parse new key
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
    char value[MAX_STRING_ARG_LEN] = { 0 };

    printf ("USAGE:\n");
    for (unsigned i = 0; i < arg_list_count; i++) {
        Argument* this = arg_list[i];
        this->interface->to_string (this->interface, value, sizeof (value));

        printf ("  %-10s\t%-20s %s ", this->name, this->interface->format_help(),
                this->description);
        if (this->is_optional) {
            printf ("(Default set to '%s')\n", value);
        } else {
            printf ("(Required)\n");
        }
    }
}

/*******************************************************************************************
 * Interfaces
 *********************************************************************************************/
#define PARENT_OF(self, type, field) ((type*)(self - offsetof (type, field)))

void generic_free (struct TypeInterface* self)
{
    if (self->value != NULL) {
        free (self->value);
    }
}

void generic_alloc (struct TypeInterface* self, va_list default_value)
{
    assert (self != NULL);

    Argument* parent = PARENT_OF (self, Argument, interface);
    assert (parent != NULL);

    if (strcmp (self->name(), "boolean") == 0) {
        self->value = malloc (sizeof (bool));
        if (!parent->is_optional) {
            *(bool*)self->value = va_arg (default_value, int);
        }
    } else if (strcmp (self->name(), "integer") == 0) {
        self->value = malloc (sizeof (int));
        if (!parent->is_optional) {
            *(int*)self->value = va_arg (default_value, int);
        }
    } else if (strcmp (self->name(), "string") == 0) {
        self->value = malloc (sizeof (char) * MAX_STRING_ARG_LEN);
        if (!parent->is_optional) {
            strncpy (self->value, va_arg (default_value, char*), MAX_STRING_ARG_LEN);
        }
    } else {
        assert (false);
    }
}

bool bool_parse_string (struct TypeInterface* self, const char* input)
{
    assert (self != NULL);
    assert (self->value != NULL);

    if (strncmp ("true", input, MAX_STRING_ARG_LEN) == 0) {
        *(bool*)self->value = true;
    } else if (strncmp ("false", input, MAX_STRING_ARG_LEN) == 0) {
        *(bool*)self->value = false;
    } else {
        fprintf (stderr, "Invalid boolean input\n");
        return false;
    }

    return true;
}

bool int_parse_string (struct TypeInterface* self, const char* input)
{
    assert (self != NULL);
    assert (self->value != NULL);

    errno              = 0; // To detect if strtol failed
    *(int*)self->value = strtol (input, NULL, 10);
    if (errno == ERANGE) {
        fprintf (stderr, "Invalid integer input\n");
        return false;
    }

    return true;
}

bool string_parse_string (struct TypeInterface* self, const char* input)
{
    assert (self != NULL);
    assert (self->value != NULL);

    strncpy (self->value, input, MAX_STRING_ARG_LEN);
    return true;
}

void bool_to_string (struct TypeInterface* self, char* out, size_t size)
{
    assert (self != NULL);
    assert (self->value != NULL);
    assert (out != NULL);
    assert (size > 0);

    bool value = *(bool*)self->value;
    strncpy (out, (value) ? "true" : "false", size);
}

void int_to_string (struct TypeInterface* self, char* out, size_t size)
{
    assert (self != NULL);
    assert (self->value != NULL);
    assert (out != NULL);
    assert (size > 0);

    int value = *(int*)self->value;

    if (value < 0 && size > 1) {
        *out++ = '-';
        size--;
        value *= -1;
    }

    char* start = out;
    for (unsigned i = 0; (i == 0 || value > 0) && size > 1 && i < size;
         i++, value /= 10, out++, size--) {
        *out = '0' + (value % 10);
    }
    *out = '\0';

    for (out--; start < out; out--, start++) {
        char t = *start;
        *start = *out;
        *out   = t;
    }
}

void string_to_string (struct TypeInterface* self, char* out, size_t size)
{
    assert (self != NULL);
    assert (self->value != NULL);
    assert (out != NULL);
    assert (size > 0);

    strncpy (out, self->value, size);
}

char* bool_format_help()
{
    return "(false|true)";
}

char* int_format_help()
{
    return "(number)";
}

char* string_format_help()
{
    return "(text)";
}

char* bool_name()
{
    return "boolean";
}

char* int_name()
{
    return "integer";
}

char* string_name()
{
    return "string";
}

/*******************************************************************************************
 * Interface types
 *********************************************************************************************/

TypeInterface Boolean = {
    .name         = bool_name,
    .alloc        = generic_alloc,
    .free         = generic_free,
    .parse_string = bool_parse_string,
    .to_string    = bool_to_string,
    .format_help  = bool_format_help,
};

TypeInterface Integer = {
    .name         = int_name,
    .alloc        = generic_alloc,
    .free         = generic_free,
    .parse_string = int_parse_string,
    .to_string    = int_to_string,
    .format_help  = int_format_help,
};

TypeInterface String = {
    .name         = string_name,
    .alloc        = generic_alloc,
    .free         = generic_free,
    .parse_string = string_parse_string,
    .to_string    = string_to_string,
    .format_help  = string_format_help,
};

/*******************************************************************************************
 * Example
 *********************************************************************************************/
int main (int argc, char** argv)
{
    char* outfile  = argument_add ("out", "Output file path", &String, true, "test.ppm");
    bool* override = argument_add ("override", "Overrides Output file if it exists", &Boolean,
                                   false);
    int* gain      = argument_add ("gain", "Gain of the amplifier", &Integer, true, 0);

    if (!argument_parse (argc, argv)) {
        print_help();
        return 1;
    }

    printf ("outfile: %s\n", outfile);
    printf ("override: %d\n", *override);
    printf ("gain: %d\n", *gain);
    return 0;
}
