#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
# define _CRT_NONSTDC_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char **argv)
{
    if (argc != 4) {
        return 1;
    }

    const char *exe    = argv[1];
    const char *output = argv[2];
    const char *input  = argv[3];
    const char *format = "%s -force -o %s - < %s";
    const size_t len   = strlen(format) + strlen(exe) + strlen(output) + strlen(input);
    char *command      = malloc(len);

    snprintf(command, len, format, exe, output, input);

    int rv = system(command);
    free(command);

    return rv;
}
