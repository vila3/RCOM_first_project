#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for close

#include "rs232.h"


int create_frame(char *frame, char ctrl){
	//int n;
	/*for(n=0;n<MAX_FRAME;n++){
		frame[n]=0x7e;
	}*/
	frame[0]=0x7e;
	frame[4]=0x7e;

	// Define endere�o
	frame[1]=0x03;

	// Define Campo Controlo
	frame[2]=ctrl;

	// BCC1 -  Block Check Character
	frame[3]=frame[1]^frame[2];


	return 1;
}

int send_frame(char *frame, char *data, int data_size){
	int n,i;
	//frame[3]=frame[1]^frame[2];

	for(n=0;n<data_size;n++){
		if(n==PAYLOAD) break;
		frame[4+n]=data[n];
	}
	if(data_size){
		frame[6+n]=0x7e; // end flag (with data)

		frame[6+n]=0; // BCC2 with XOR across data
		for(i=0;i<data_size;i++){
			frame[6+n]^=frame[4+i];
		}
		n = write(fd,frame,n+7);//era 7
	}
	else{
		frame[4]=0x7e; // end flag without data
		n = write(fd,frame,5);
	}
	return n;
}

int receive_frame(int fd, char* buff, int buff_size) {

    printf("\nWaiting transmission...\n");

	char tmp;

	/* Waiting for flag */
	int i=0, init_frame=FALSE;
	
    while (init_frame==FALSE) {       /* loop for input */
      read(fd,&tmp,1);
      if (tmp== 0x7E) init_frame=TRUE;
    }
	/* verify repeated flag */
	read(fd, &tmp, 1);
    if (tmp != 0x7E) buff[i++] = tmp;
	printf("Char em tmp: %c\n",tmp);
	while (i < buff_size) {
		read(fd, &tmp, 1);
    	if (tmp == 0x7E) break;
		printf("%d %d\n", i , buff_size);
    	buff[i++] = tmp;
		printf("Char em tmp: %c\n",tmp);
	}
	printf("saiu da merda\n");
	return i;
}

int read_frame(char* frame, char* data, char* from_address, char *ctrl) {
	int i=0;

	*from_address = frame[i++];
	*ctrl = frame[i++];


	if ((*from_address ^ *ctrl) != frame[i++]) {
		return -1;
	}

	// The code bellow is to check BCC2,
	// but with the current implementation (without flags after receive_frame)
	// we don't have a way to check it

	/*if(frame[i]!=0x7e){ // Check BCC2
	
		for(i=0;i<data_size-1;i++){
			frame[6+n]^=frame[4+i];
		}
		n = write(fd,frame,n+7);
	}*/
	if(data!=NULL){
		for(i=0;;i++){
			data[i]=frame[i+3];
			if(data[i]=='\0') break;
		}
	}

	return 0;
}

int llopen(char* serial_port, int mode) {

    char buf[PAYLOAD];
    char frame1[MAX_FRAME]={0x7e};


    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */


    fd = open(serial_port, O_RDWR | O_NOCTTY );
    if (fd <0) {perror(serial_port); exit(-1); }

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

    newtio.c_cc[VTIME]    = 5;   /* inter-character timer unused */
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

	if (mode == MODE_WRITE) {
		if(create_frame(frame1,CTRL_SET))
		{
			send_frame(frame1, NULL, 0);
		}

		int n;
		while(n==0){
			n =	receive_frame(fd, buf, MAX_FRAME);
		}

		char from_address, ctrl;
		n = read_frame(buf, NULL, &from_address, &ctrl);

		if (n == 0 && ctrl == CTRL_UA) {
			printf("Connection open, ready to write!\n");
		} 
		else {
			printf("An error occur opening the connection!\n");
			return -1;
		}
	} 
	else {
		while(llread()==0);
		printf("Connection open, ready to read!\n");
	}
	return 1;
}

int llread() {
	char buf[MAX_FRAME], data[PAYLOAD], from_address, ctrl;

	int n;
	n =	receive_frame(fd, buf, MAX_FRAME);

	n = read_frame(buf, data, &from_address, &ctrl);

	if (ctrl == CTRL_SET) {
		char frame1[MAX_FRAME];
		if(create_frame(frame1, CTRL_UA))
		{
			send_frame(frame1, NULL, 0);
		}
	} 
	else {
		printf("%s\n", data);
	}

	return n;
}

int llclose() {
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);

	printf("Connection closed!\n");

	return 1;
}

int llwrite(char *data){
	// Include a string in a frame and send it
	char frame1[MAX_FRAME]={0x7e};
	if(create_frame(frame1,0x00))
	{
		send_frame(frame1,data,strlen(data)+1);
	}
	return 1;
}
