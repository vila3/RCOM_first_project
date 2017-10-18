
/*Non-Canonical Input Processing*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rs232.h"

int main(int argc, char** argv)
{
	if ( (argc < 2) ||
  	     (strstr(argv[1], "/dev/") == NULL)) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

    llopen(argv[1], TRANSMITTER);

	printf("Send string: \"String to ~ send!\"\n");
	sleep(4000);
	llwrite("String to ~ send!");

	llclose();

	return 0;
}
