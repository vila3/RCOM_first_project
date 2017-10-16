/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include "rs232.h"

int main(int argc, char** argv)
{
    llopen(argv[1], MODE_READ);

    printf("Start reading...\n");
    char *data = NULL;
    llread(&data);
    printf("End read!\n");

    printf("%s\n", data);

    llclose();

    return 0;
}
