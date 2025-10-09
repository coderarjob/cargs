## cargs

Inspired by Tsoding's flag.h, this is my implementation for the library from scratch. It uses
interface for parsing various types of arguments, thus can be easily extended without changing the
source code in the library.

* The implementation codes with support for `Integer`, `Boolean`, `String` and `Double` type of
arguments.
* Support for optional with default arguments
* Support for boolean flags type of arguments
* Extendable for other types, allowing conversion to even custom types while parsing input
  arguments.

Here is an example:

```c
int main (int argc, char** argv)
{
    char* ouput_file = argument_add("O", "Output file path", String, false);
    bool* should_override = argument_add("override", "Override output file", Flag, true, false);

    if (!argument_parse (argc, argv)) {
        print_help();
        return 1;
    }

    printf ("output_file: %s\n", output_file);
    printf ("override: %d\n", *override);
}
```

Reference: https://github.com/tsoding/flag.h

### Versioning

It uses semantic versioning. See [https://semver.org/](https://semver.org).


### Feedback

Open a GitHub issue or drop a email at arjobmukherjee@gmail.com. I would love to hear your
suggestions and feedbacks.
