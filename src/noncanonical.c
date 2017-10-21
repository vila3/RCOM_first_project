/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>

#include "rs232.h"

int main(int argc, char** argv)
{
    int port, n, fd;

    port=llopen(argv[1], RECEIVER);

    fd = open(argv[2], O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IXUSR);


    char *data;

    do {
      n = llread(port,&data);
      if (n<=0) break;
      write(fd, data, n);
      free(data);
      data = NULL;
    } while(1);

    llclose(port);

    return 0;
}
