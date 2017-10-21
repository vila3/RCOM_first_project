/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include "rs232.h"

int main(int argc, char** argv)
{
    int fd, n;

    fd=llopen(argv[1], RECEIVER);

    char *data = NULL;

    do {
      n = llread(fd,&data);
    } while(n>0);

    llclose(fd);

    return 0;
}
