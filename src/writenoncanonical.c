/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rs32.h"

volatile int STOP=FALSE;


int cria_trama(char *frame, char ctrl){
	int n;
	for(n=0;n<MAX_FRAME;n++){
		frame[n]=0x7e;	
	}

	// Define endereço	
	frame[1]=0x03;

	// Define Campo Controlo
	frame[2]=ctrl;
	
	/*for(n=0;data[n]!="\0";n++){
		frame[4+n]=data[n];
		if(n=MAX_FRAME-6){
			frame[2]=frame[2]^0x40;
			frame[2]=frame[2]+0x80;
		}	
	}*/

	return n;
}
int envia_trama(char *frame, char *data, int data_size){
	int n;
	frame[3]=frame[1]^frame[2];
	printf("Data: %s\n",data);
	for(n=0;n<data_size;n++){
		if(n==MAX_FRAME-6) break;
		frame[4+n]=data[n];
	}
	return n;
}


int main(int argc, char** argv)
{
    int fd,c, res,n=0,s=0;
    struct termios oldtio,newtio;
    char buf[PAYLOAD],data[PAYLOAD];
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
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

	// Código aula 2

	printf("Data to send: ");
	//s=scanf("%s",data);
	s=fgets(data,255,stdin);
	puts(data);

	if(cria_trama(frame1,0x03))
	{
		envia_trama(frame1,data,strlen(data));
	}
	//printf("Primeiro valor da trama: %x\n",frame1[0]);
	res=write(fd,frame1,MAX_FRAME);


	// - fim código aula 2

	/*
	gets(buf);
	//printf("%s\n",buf);   
	
    for (i = 0; i < 255; i++) {

      if(buf[i] == 0) break;
    }
    i++;
    
    res = write(fd,buf,i);  
    printf("%d bytes written\n", res);
 	*/
	sleep(1);

  /* 
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar 
    o indicado no guião 
  */



   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }




    close(fd);
    return 0;
}
