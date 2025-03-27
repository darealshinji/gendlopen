#include <sstream>
#include <stdlib.h>


int main(int argc, char **argv)
{
    if (argc != 4) {
        return 1;
    }

    const char *exe = argv[1];
    const char *input = argv[2];
    const char *output = argv[3];

    std::stringstream command;
    command << exe << " -force -o " << output << " - < " << input;

    return system(command.str().c_str());
}
