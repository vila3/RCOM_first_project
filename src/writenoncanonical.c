/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rs32.h"

volatile int STOP=FALSE;
volatile int INIT=FALSE;


int cria_trama(char *frame, char ctrl){
	int n;
	for(n=0;n<MAX_FRAME;n++){
		frame[n]=0x7e;
	}

	// Define endereço
	frame[1]=ADRESS_SEND;

	// Define Campo Controlo
	frame[2]=ctrl;

	// BCC1 -  Block Check Character
	frame[3]=frame[1]^frame[2];
	return n;
}
int envia_trama(char *frame, char *data, int data_size){
	int n;
	printf("Data: %s\n",data);
	for(n=0;n<data_size;n++){
		if(n==MAX_FRAME-6) break;
		frame[4+n]=data[n];
	}
	return n;
}
int receive_frame(int fd, char* buff, int buff_size) {

  printf("\nWaiting transmission...\n");

	char tmp;

	/* Waiting for flag */
	int i=0, res;
    while (INIT==FALSE) {       /* loop for input */
      res = read(fd,&tmp,1);
      //printf("%c\n", tmp);
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
		//printf("%c\n", buff[i-1]);
	}

	//printf("%s\n", buff);
	return i;
}
int read_frame(char* frame, char* package, char* from_address) {
	int i=0;

	*from_address = frame[i];

	return 0;
}

int main(int argc, char** argv)
{
    int fd,c, res,n=0,s=0;
    struct termios oldtio,newtio;
    char buf[MAX_FRAME],data[PAYLOAD];
    int i, sum = 0, speed = 0;

	char frame1[MAX_FRAME]={0x7e};


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
    newtio.c_oflag = 0; //BAUDRATE | CS8;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */

  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) pr�ximo(s) caracter(es)
  */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

	// Code for lab1

	// Connect
	cria_trama(frame1,CTRL_SET);
	res=write(fd,frame1,MAX_FRAME);

	// Waits response from receiver
	char *package, from_address;
	n =	receive_frame(fd, frame1, MAX_FRAME);
	if(frame1[2]==CTRL_UA)
		printf("-- Connection accepted --\n");



	// Include a string in a frame and send it
	printf("Data to send: ");
	s=fgets(data,PAYLOAD,stdin);
	if(cria_trama(frame1,0x00))
	{
		envia_trama(frame1,data,strlen(data));
	}
	res=write(fd,frame1,MAX_FRAME);

	//Disconnect
	cria_trama(frame1,CTRL_DISC);
	res=write(fd,frame1,MAX_FRAME);

	// - end code from lab1

	sleep(1);

  /*
    O ciclo FOR e as instru��es seguintes devem ser alterados de modo a respeitar
    o indicado no gui�o
  */

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    return 0;
}
