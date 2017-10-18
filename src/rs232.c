#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for close
#include <signal.h> // for timeout

#include "rs232.h"

int debugging = 0;
int flag=1, attempts=1;
static unsigned char ctrl_state=0;

void timeout_handler()                   // answer alarm
{
	printf("Passaram 3 segundos: # %d, vamos tentar novamente\n", attempts);
	flag=1;
	attempts++;
}

void print_frame(char *frame, int len) {
	printf("Frame: ");
	int i;
	for (i = 0; i < len; i++) {
		printf("%x", frame[i]);
	}
	printf("\n");
	printf("Frame: ");
	for (i = 0; i < len; i++) {
		printf("%c", frame[i]);
	}
	printf("\n");
}

int destuffing(char **frame, int frame_len) {
	int i, n_escape=0, n=0;
	char c;
	for (i = 0; i < frame_len; i++) {
		if ((*frame)[i] == 0x7d) n_escape++;
	}
	char *new_frame = malloc( sizeof(char) * frame_len - n_escape);

	for (i = 0; i < frame_len; i++) {
		c = (*frame)[i];
		if (c == 0x7d) c = (*frame)[++i]^0x20;

		new_frame[n++] = c;
	}

	*frame = new_frame;
	return n;
}

int stuffing(char **frame, int frame_len){
	int i,n_escape=0,stuffed_frame_len=0,n=0;

	for(i = 1; i < frame_len-1; i++){
		if((*frame)[i] == 0x7e)
			n_escape++;
			//printf("\nNumero de escapes:%d\n",n_escape);
	}

	if (n_escape == 0) {
		return frame_len;
	}

	stuffed_frame_len=frame_len+n_escape;
	char *stuffed_frame = (char*) malloc(sizeof(char)*stuffed_frame_len);

	for(i = 0; i < frame_len; i++){
		if((*frame)[i] == 0x7e && i>0 && i<frame_len-1){
			stuffed_frame[n+1] = 0x20^(*frame)[i];
			stuffed_frame[n] = 0x7d;
			n += 2;
		}
		else{
			stuffed_frame[n++]=(*frame)[i];
		}
	}
	*frame=stuffed_frame;

	return stuffed_frame_len;
}

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
	int n,i,frame_size;

	for(n=0;n<data_size;n++){
		if(n==PAYLOAD) break;
		frame[4+n]=data[n];
	}
	if(data_size){

		frame[4+n]=frame[4]; // BCC2 with XOR across data
		for(i=1;i<data_size;i++){
			frame[4+n]^=frame[4+i];
		}

		frame[5+n]=0x7e; // end flag (with data)

		frame_size=n+6;
	}
	else{
		frame[4]=0x7e; // end flag without data
		frame_size=5;
	}

	frame_size=stuffing(&frame,frame_size);

	n = write(fd,frame,frame_size);
	return n;
}

int receive_frame(int fd, char** buff, int buff_size) {

	*buff = malloc(sizeof(char) * MAX_FRAME);
	if (debugging)
    	printf("\nWaiting transmission...\n");

	char tmp;

	/* Waiting for flag */
	int i=0, init_frame=FALSE;

    while (init_frame==FALSE) {       /* loop for input */
      read(fd,&tmp,1);
      if (tmp== 0x7E)
			{
				init_frame=TRUE;
				break;
			}
      return -1;
    }
	/* verify repeated flag */
	read(fd, &tmp, 1);
    if (tmp != 0x7E) (*buff)[i++] = tmp;
	while (i < buff_size) {
		read(fd, &tmp, 1);
    	if (tmp == 0x7E) break;
    	(*buff)[i++] = tmp;
	}

	i = destuffing(buff, i);

	return i;
}

int read_frame(char* frame, int frame_len, char* data, char* from_address, char *ctrl) {
	int i=0;

	*from_address = frame[i++];
	*ctrl = frame[i++]>>6;

	if ((*from_address ^ *ctrl) != frame[i++]) {
		return -1;
	}

	// The code bellow is to check BCC2,

	char bcc2 = frame[frame_len-1], bcc_check = frame[3];

	for(i=1;i<frame_len-4;i++){
		bcc_check^=frame[3+i];
	}

	if (data == NULL) return 0;

	if (bcc2 == bcc_check) {
		for(i=0; i<frame_len-4;i++){
			data[i]=frame[i+3];
		}

		data[i] = 0;


		return i;
	} else {
		if (debugging)
			printf("Bcc2 fail\n");
	}

	// TODO pedir trama novamente
	return 0;
}

int llopen(char* serial_port, int mode) {
    char *buf;
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
    newtio.c_cc[VMIN]     = 0;   /* blocking read until N chars received */

    /*
      VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
      leitura do(s) pr�ximo(s) caracter(es)
    */
    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

	int n=-1;
	char from_address, ctrl;
	if (mode == TRANSMITTER) {
		// Timeout implementation
		(void) signal(SIGALRM, timeout_handler);  // instala  rotina que atende interrupcao
		do {
			if(create_frame(frame1,CTRL_SET))
			{
				send_frame(frame1, NULL, 0);
				// Start timer
				if(flag){
					alarm(3);	// activate timer of 3s
					flag=0;
				}
			}

			while(n<0 && !flag)
			{
				n =	receive_frame(fd, &buf, MAX_FRAME);
			}

			if(flag)	continue;
			n = read_frame(buf, n, NULL, &from_address, &ctrl);
		}
		while (n != 0 && ctrl != CTRL_UA && attempts < MAX_ATTEMPTS);

		if(n == 0 && ctrl == CTRL_UA) {
			attempts = 0;
			if (debugging)
				printf("Connection open, ready to write!\n");
		}
		else {
			if (debugging)
				printf("An error occur opening the connection!\n");
			return -1;
		}
	}
	else {
		while(1) {
			n = receive_frame(fd, &buf, MAX_FRAME);
			n = read_frame(buf, n, NULL, &from_address, &ctrl);

			if (ctrl == CTRL_SET) {
				char frame1[MAX_FRAME];
				if(create_frame(frame1, CTRL_UA))
				{
					send_frame(frame1, NULL, 0);
				}

				break;
			}
		}
		if (debugging)
			printf("Connection open, ready to read!\n");
	}
	return 1;
}

int llread(char** buff) {
	char *buf, from_address, ctrl, frame1[MAX_FRAME] = {0x7e};
	int n;
	char *data;

	do {
		n=-1;
		while (n<0) {
			n =	receive_frame(fd, &buf, MAX_FRAME);
		}

		data = (char *) malloc( sizeof(char) * ( n - 3 ) );
		n = read_frame(buf, n, data, &from_address, &ctrl);

		if(create_frame(frame1, ctrl))
		{
			send_frame(frame1,NULL,CTRL_RR);
		}
	} while(ctrl_state != ctrl);

	ctrl_state = (ctrl_state+1)%2;
	*buff = data;

	return n;
}

int llclose() {
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);

	if (debugging)
		printf("Connection closed!\n");

	return 1;
}

int llwrite(char *data){
	int n=-1;
	char *buf;
	char from_address, ctrl;
	// Include a string in a frame and send it
	char frame1[MAX_FRAME]={0x7e};
	do{
		if(create_frame(frame1, ( ctrl_state << 6 )) )
		{
			send_frame(frame1,data,strlen(data)+1);
			// Start timer
			if(flag){
				alarm(3);	// activate timer of 3s
				flag=0;
			}
			while(n<0 && !flag)
			{
				n =	receive_frame(fd, &buf, MAX_FRAME);
			}
			if(flag)	continue;
			n = read_frame(buf, n, NULL, &from_address, &ctrl);
		}
	}
	while( (n <= 0 || ctrl != (ctrl_state+1)%2 ) && attempts < MAX_ATTEMPTS);
	if(attempts >= MAX_ATTEMPTS) return -1;
	ctrl_state = (ctrl_state+1)%2;
	return 1;
}
