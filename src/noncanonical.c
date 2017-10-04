/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define BUFF_SIZE 512

volatile int INIT=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[BUFF_SIZE], *package, from_address;

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 5;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

	
	int n;
	n =	receive_frame(fd, buf, BUFF_SIZE);
	
	read_frame(buf, package, &from_address);
	

	printf("%s\n", buf);



  /* 
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião 
  */



    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}

int receive_frame(int fd, char* buff, int buff_size) {
	
    printf("\nWaiting transmission...\n");
	    
	unsigned char tmp;
	
	/* Waiting for flag */
	int i=0, res;
    while (INIT==FALSE && i < buff_size) {       /* loop for input */
      res = read(fd,&tmp,1);  
      if (tmp== 0x7E) INIT=TRUE;
    }

	/* verify repeated flag */
	i += 1;
	res = read(fd, &tmp, 1);  
    if (tmp != 0x7E) buff[i++] = tmp;
	
	while (i < buff_size) {
		res = read(fd, &tmp, 1);  
    	if (tmp == 0x7E) break;
    	buff[i++] = tmp;
	}
	
	return i;
}

int read_frame(char* frame, char* package, char* from_address) {
	int i=0;
	
	*from_address = frame[i];
	
	return 0;
}
