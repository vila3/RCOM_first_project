#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define MAX_FRAME 512
#define CTRL_SET 0x03
#define CTRL_DISC 0x0B
#define CTRL_UA 0x07
#define CTRL_RR 0x05
#define CTRL_REJ 0x01
#define MAX_FLAGS 6
#define PAYLOAD MAX_FRAME-MAX_FLAGS
#define MAX_ATTEMPTS 3

#define TRANSMITTER 1
#define RECEIVER 0

int fd;
struct termios oldtio,newtio;

int llopen(char* serial_port, int mode);

int llread();

int llclose();

int llwrite(char *data);
