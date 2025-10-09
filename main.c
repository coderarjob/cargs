#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define MAX_NAME_LEN        50  // null byte is not included
#define MAX_DESCRIPTION_LEN 100 // null byte is not included
#define MAX_INPUT_VALUE_LEN 100 // null byte is not included

typedef struct TypeInterface {
    void* value;
    char* name;
    char* format_help;
    size_t type_size;
    void (*alloc) (struct TypeInterface* self, const char* default_value);
    void (*free) (struct TypeInterface* self);
    bool (*parse_string) (struct TypeInterface* self, const char* input);
} TypeInterface;

typedef struct {
    char* name;
    char* description;
    char* default_value;
    bool provided; // true if some value was provided. Always true for optional arguments.
    TypeInterface interface;
} Argument;

void panic (const char* msg)
{
    if (msg != NULL) {
        fprintf (stderr, "PANIC! %s\n", msg);
    }
    exit (1);
}

#define ERROR(ret, msg, ...)                                 \
    do {                                                     \
        fprintf (stderr, "ERROR: " msg "\n", ##__VA_ARGS__); \
        return ret;                                          \
    } while (0)

/*******************************************************************************************
 * Argument functions
 *********************************************************************************************/
unsigned int arg_list_count = 0;
Argument* arg_list[10];

void* argument_add (const char* name, const char* description, TypeInterface interface,
                    const char* default_value)
{
    Argument* new = NULL;

    if (arg_list_count >= 10) {
        panic ("Too many arguments added");
    }

    if (!(new = malloc (sizeof (Argument)))) {
        perror ("ERROR: Allocation failed");
        panic (NULL);
    }

    char temp_name[MAX_NAME_LEN + 1] = { '-', 0 }; // +1 for '\0' char
    strncat (temp_name, name, MAX_NAME_LEN - 1);   // -1 because '-' char is already in

    if (!(new->name = strndup (temp_name, MAX_NAME_LEN))) {
        perror ("ERROR: Allocation failed");
        panic (NULL);
    }

    if (!(new->description = strndup (description, MAX_DESCRIPTION_LEN))) {
        perror ("ERROR: Allocation failed");
        panic (NULL);
    }

    if (default_value != NULL) {
        if (!(new->default_value = strndup (default_value, MAX_DESCRIPTION_LEN))) {
            perror ("ERROR: Allocation failed");
            panic (NULL);
        }
    } else {
        new->default_value = NULL;
    }

    new->provided  = default_value != NULL;
    new->interface = interface;
    new->interface.alloc (&new->interface, default_value);

    arg_list[arg_list_count++] = new;

    return new->interface.value;
}

void argument_free (Argument* arg)
{
    arg->interface.free (&arg->interface);
    free (arg->name);
    free (arg->description);
    if (arg->default_value) {
        free (arg->default_value);
    }
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
                ERROR (false, "Argument '%s' is unknown", arg);
            }
            state_is_key = false; // Now parse value for this key

            // Special case for flags. They are the only ones with just key and no value, so it made
            // little sense to add a 'is_flag' field in the TypeInterface struct.
            if (strcmp ("flag", this->interface.name) == 0) {
                this->provided = this->interface.parse_string (&this->interface, arg);
                state_is_key   = true; // Now parse value for this key
            }
        } else {
            assert (this != NULL);
            assert ((this->default_value && this->provided) ||
                    (!this->default_value && !this->provided));

            this->provided = this->interface.parse_string (&this->interface, arg);
            state_is_key   = true; // Now parse new key
        }
    }

    assert (state_is_key == true);

    for (unsigned i = 0; i < arg_list_count; i++) {
        Argument* this = arg_list[i];
        if (!this->provided) {
            ERROR (false, "Argument '%s' is required but was not provided", this->name);
        }
    }
    return true;
}

void print_help()
{
    printf ("USAGE:\n");
    for (unsigned i = 0; i < arg_list_count; i++) {
        Argument* this = arg_list[i];

        printf ("  %-10s\t%-20s %s ", this->name, this->interface.format_help, this->description);
        if (this->default_value) {
            printf ("(Default set to '%s')\n", this->default_value);
        } else {
            printf ("(Required)\n");
        }
    }
}

/*******************************************************************************************
 * Interfaces
 *********************************************************************************************/
#define PARENT_OF(self, type, field) ((type*)((uintptr_t)self - offsetof (type, field)))

void generic_free (struct TypeInterface* self)
{
    if (self->value != NULL) {
        free (self->value);
    }
}

void default_alloc (struct TypeInterface* self, const char* default_value)
{
    if (!(self->value = malloc (self->type_size))) {
        perror ("ERROR: Allocation failed");
        panic (NULL);
    }

    if (default_value != NULL) {
        self->parse_string (self, default_value);
    }
}

bool bool_parse_string (struct TypeInterface* self, const char* input)
{
    assert (self != NULL);
    assert (self->value != NULL);

    if (strncmp ("true", input, MAX_INPUT_VALUE_LEN) == 0) {
        *(bool*)self->value = true;
    } else if (strncmp ("false", input, MAX_INPUT_VALUE_LEN) == 0) {
        *(bool*)self->value = false;
    } else {
        ERROR (false, "Invalid boolean input");
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
        ERROR (false, "Invalid integer input");
    }

    return true;
}

bool string_parse_string (struct TypeInterface* self, const char* input)
{
    assert (self != NULL);
    assert (self->value != NULL);

    strncpy (self->value, input, MAX_INPUT_VALUE_LEN);
    return true;
}

bool flag_parse_string (struct TypeInterface* self, const char* input)
{
    assert (self != NULL);
    assert (self->value != NULL);

    (void)input;

    *(bool*)self->value = true;
    return true;
}

bool double_parse_string (struct TypeInterface* self, const char* input)
{
    assert (self != NULL);
    assert (self->value != NULL);

    errno                 = 0; // To detect if strtol failed
    *(double*)self->value = strtod (input, NULL);
    if (errno == ERANGE) {
        ERROR (false, "Invalid floating point input");
    }

    return true;
}

/*******************************************************************************************
 * Interface types
 *********************************************************************************************/

TypeInterface Boolean = {
    .name         = "boolean",
    .type_size    = sizeof (int),
    .alloc        = default_alloc,
    .free         = generic_free,
    .parse_string = bool_parse_string,
    .format_help  = "(false|true)",
};

TypeInterface Integer = {
    .name         = "integer",
    .type_size    = sizeof (int),
    .alloc        = default_alloc,
    .free         = generic_free,
    .parse_string = int_parse_string,
    .format_help  = "(number)",
};

TypeInterface String = {
    .name         = "string",
    .type_size    = sizeof (char) * MAX_INPUT_VALUE_LEN,
    .alloc        = default_alloc,
    .free         = generic_free,
    .parse_string = string_parse_string,
    .format_help  = "(text)",
};

TypeInterface Flag = {
    .name         = "flag",
    .type_size    = sizeof (int),
    .alloc        = default_alloc,
    .free         = generic_free,
    .parse_string = flag_parse_string,
    .format_help  = "",
};

TypeInterface Double = {
    .name         = "double",
    .type_size    = sizeof (double),
    .alloc        = default_alloc,
    .free         = generic_free,
    .parse_string = double_parse_string,
    .format_help  = "(decimal number)",
};

/*******************************************************************************************
 * Example
 *********************************************************************************************/
typedef enum {
    MODE_SINE_WAVE = 0,
    MODE_AM_WAVE   = 1,
    MODE_NOISE     = 2,
    MODES_COUNT
} Modes;

bool modes_parse_string (struct TypeInterface* self, const char* input)
{
    assert (self != NULL);

    Modes mode = 0;
    if (strncmp ("sine", input, MAX_INPUT_VALUE_LEN) == 0) {
        mode = MODE_SINE_WAVE;
    } else if (strncmp ("am", input, MAX_INPUT_VALUE_LEN) == 0) {
        mode = MODE_AM_WAVE;
    } else if (strncmp ("noise", input, MAX_INPUT_VALUE_LEN) == 0) {
        mode = MODE_NOISE;
    } else {
        ERROR (false, "Invalid mode: '%s'", input);
    }

    *(Modes*)self->value = mode;
    return true;
}

TypeInterface ModesInterface = {
    .name         = "Modes",
    .alloc        = default_alloc,
    .type_size    = sizeof (Modes),
    .free         = generic_free,
    .parse_string = modes_parse_string,
    .format_help  = "(sine|am|noise)",
};

int main (int argc, char** argv)
{
    char* outfile  = argument_add ("out", "Output file path", String, "test.ppm");
    bool* override = argument_add ("override", "Overrides Output file if it exists", Boolean, NULL);
    int* gain      = argument_add ("gain", "Gain of the amplifier", Integer, "0");
    bool* root     = argument_add ("R", "Run the application as root", Flag, "false");
    double* offset = argument_add ("offset", "Offset for the amplifer", Double, "13.6");
    Modes* mode    = argument_add ("mode", "Mode of wave generation", ModesInterface, NULL);

    if (!argument_parse (argc, argv)) {
        print_help();
        return 1;
    }

    printf ("outfile: %s\n", outfile);
    printf ("override: %d\n", *override);
    printf ("gain: %d\n", *gain);
    printf ("root: %d\n", *root);
    printf ("offset: %f\n", *offset);
    printf ("mode: %d\n", *mode);

    for (unsigned i = 0; i < arg_list_count; i++) {
        Argument* this = arg_list[i];
        argument_free (this);
    }
    return 0;
}
