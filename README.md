<p align="left">
    <img src="/docs/logo.svg"/>
    <br/>
    <img src="https://github.com/coderarjob/cargs/actions/workflows/build.yaml/badge.svg"/>
</p>

## cargs

Cargs came about inspired by Tsoding's [flag.h](https://github.com/tsoding/flag.h). This however is
not a fork, it takes only the idea as inspiration, the implementation, API is fully custom and
written from scratch. 

When you add an argument, you must also specify its type. This type determines what kind of input is
accepted and how the input command line is parsed. Common types like `Integer`, `Boolean`, `String`
and `Double` type of arguments are provided built-in, but one can also create custom types. See
Examples.

* Optional arguments with default values
* Boolean flags type of arguments
* Multiple values is accepted if type is wrapped with `CARGS_LISTOF`.
* Conditional arguments. There are arguments which are enabled when condition is met.

Here is an example:

```c
#include <stdio.h>
#define CARGS_IMPLEMENTATION
#include "./cargs.h"

typedef struct {
    bool* list;
    int* number_of_columns;
    bool* show_hidden;
    bool* help;
    Cargs_ArrayList* dir;
} Config;

Config config;

bool number_columns_is_enabled (void)
{
    return *config.list == true;
}

int main (int argc, char** argv)
{
    config.dir         = cargs_add_arg ("dir", "List this directory", CARGS_LISTOF (String), ".");
    config.list        = cargs_add_arg ("l", "Show as list", Flag, "false");
    config.show_hidden = cargs_add_arg ("hidden", "Show hidden files", Flag, "false");
    config.help        = cargs_add_arg ("h", "Show help", Help, "false");
    config.number_of_columns = cargs_add_cond_arg_d (number_columns_is_enabled, "When -l is given",
                                                     "L", "Number of list columns", Integer, "1");
    if (!cargs_parse_input (argc, argv)) {
        cargs_print_help();
        return 1;
    }

    if (*config.help) {
        cargs_print_help();
        return 0;
    }

    printf ("Dirs:\n  Count:%ld\n", config.dir->len);

    for (unsigned i = 0; i < config.dir->len; i++) {
        printf ("  %d: %s\n", i, ((Cargs_StringType*)config.dir->buffer)[i]);
    }
    printf ("show as list: %d\n", *config.list);
    printf ("show hidden files: %d\n", *config.show_hidden);
    if (number_columns_is_enabled()) {
        printf ("Number of columns (listing): %d\n", *config.number_of_columns);
    }

    return 0;
}
```

<img src="/docs/screenshot_help.png"/>

### Versioning

It uses semantic versioning. See [https://semver.org/](https://semver.org).

### Feedback

Open a GitHub issue or drop a email at arjobmukherjee@gmail.com. I would love to hear your
suggestions and feedbacks.
