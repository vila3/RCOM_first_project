#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for close
#include <signal.h> // for timeout
// #include <time.h>
#include <sys/time.h>

#include "rs232.h"

int debugging = 0;
int flag=1, attempts=0, stop=0, interrupt_alarm=0;
int n_bytes=0;

static char ctrl_state=0;

void timeout_handler()                   // answer alarm
{
	if(!interrupt_alarm){
			printf("3 seconds have passed: # %d, tries left: %d\n", attempts+1,MAX_ATTEMPTS-attempts-1);
		flag=1;
		attempts++;
	}

}

void print_frame(char *frame, int len) {
	printf("Frame: ");
	int i;
	for (i = 0; i < len; i++) {
		printf("%x ", frame[i]);
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

// falta também verificar o 7d, não é só o 7e
int stuffing(char **frame, int frame_len){
	int i,n_escape=0,stuffed_frame_len=0,n=0;

	for(i = 1; i < frame_len-1; i++){
		if( (*frame)[i] == 0x7e || (*frame)[i] == 0x7d )
			n_escape++;
			//printf("\nNumero de escapes:%d\n",n_escape);
	}

	if (n_escape == 0) {
		return frame_len;
	}

	stuffed_frame_len=frame_len+n_escape;
	char *stuffed_frame = (char*) malloc(sizeof(char)*stuffed_frame_len);

	for(i = 0; i < frame_len; i++){
		if(( (*frame)[i] == 0x7e || (*frame)[i] == 0x7d) && i>0 && i<frame_len-1){
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

int send_frame(int fd, char *frame, char *data, int data_size){
	int n, frame_size;
	char bcc2;

	for(n=0;n<data_size;n++){
		if(n==PAYLOAD) break;
		frame[4+n]=data[n];
		if (n==0) {
			bcc2 = data[n];
		} else {
			bcc2 ^= data[n];
		}
	}
	if(data_size){

		frame[4+n]=bcc2;
		//printf("bcc2 enviado: %x \n",frame[4+n]);
		frame[5+n]=0x7e; // end flag (with data)

		frame_size=n+6;
	}
	else{
		frame[4]=0x7e; // end flag without data    -> acho que está feito na create_frame
		frame_size=5;
	}

	frame_size=stuffing(&frame,frame_size);

	write(fd,frame,frame_size); // TODO prever quando  acabar mais cedo

	return n;
}


int receive_frame(int fd, char** buff) {

	*buff = malloc(sizeof(char) * 2 * MAX_FRAME);

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
	while (i < 2 * MAX_FRAME) {
		read(fd, &tmp, 1);
    	if (tmp == 0x7E) break;
    	(*buff)[i++] = tmp;
	}

	n_bytes+=i;

	i = destuffing(buff, i);

	return i;
}

int read_frame(char* frame, int frame_len, char* data, char* from_address, char *ctrl) {
	int i=0;

	*from_address = frame[i++];
	*ctrl = frame[i++];

	int rand_bcc1_fail = (rand() % 100) + 1;
	if ((*from_address ^ *ctrl) != frame[i++] || rand_bcc1_fail <= BCC1_FAIL_RATE ) {
		if (debugging)
			printf("Bcc1 fail\n");
		return -1;
	}

	// The code bellow is to check BCC2,

	char bcc2 = frame[frame_len-1], bcc_check = frame[3];

	for(i=1;i<frame_len-4;i++){
		bcc_check^=frame[3+i];
	}

	if (data == NULL || frame_len < 4) return 0;

	int rand_bcc2_fail = (rand() % 100) + 1;
	if (bcc2 == bcc_check && rand_bcc2_fail > BCC2_FAIL_RATE) {
		for(i=0; i<frame_len-4;i++){
			data[i]=frame[i+3];
		}

		return i;
	} else {
		if (debugging)
			printf("Bcc2 fail\n");
	}

	// TODO pedir trama novamente
	return -1;
}

int llopen(char* serial_port, int mode) {
    char *buf;
    char frame1[MAX_FRAME]={0x7e};
		int fd;


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
				send_frame(fd, frame1, NULL, 0);
				// Start timer
				if(flag){
					interrupt_alarm=0;
					alarm(3);	// activate timer of 3s
					flag=0;
				}
			}

			while(n<0 && !flag)
			{
				n =	receive_frame(fd, &buf);
			}

			if(flag)	continue;
			flag=interrupt_alarm=1;
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
		if (debugging)
	    	printf("\nWaiting transmission...\n");
		while(1) {
			while (n<0) {
				n =	receive_frame(fd, &buf);
			}

			n = read_frame(buf, n, NULL, &from_address, &ctrl);

			if (ctrl == CTRL_SET) {
				char frame1[MAX_FRAME];
				if(create_frame(frame1, CTRL_UA))
				{
					send_frame(fd, frame1, NULL, 0);
				}

				break;
			}
		}
		if (debugging)
			printf("Connection open, ready to read!\n");
	}
	return fd;
}

int llread(int fd, char* buff) {
	char *buf, from_address, ctrl, frame1[2*MAX_FRAME] = {0x7e};
	int n;

	if (debugging)
			printf("\nWaiting transmission...\n");
	do {
		buf=NULL;
		n=-1;
		while (n<0) {
			n =	receive_frame(fd, &buf);
		}

		usleep(1000*TPROP);
		n = read_frame(buf, n, buff, &from_address, &ctrl);

		if (ctrl == CTRL_DISC) {
			if(create_frame(frame1, CTRL_DISC))
			{
				send_frame(fd, frame1,NULL,0);
			}

			do {
				do {
					n =	receive_frame(fd, &buf);
				} while (n<0);

				n = read_frame(buf, n, NULL, &from_address, &ctrl);

				if (ctrl == CTRL_DISC) {
					if(create_frame(frame1, CTRL_DISC))
					{
						send_frame(fd, frame1,NULL,0);
					}
				}

			} while(n<0 || ctrl != CTRL_UA);

			printf("Total bytes = %d\n",n_bytes);
			stop=1;
			return 0;
		} else if (ctrl == CTRL_SET) {
			char frame1[MAX_FRAME];
			if(create_frame(frame1, CTRL_UA))
			{
				send_frame(fd, frame1, NULL, 0);
			}

			continue;
		}

		ctrl = ctrl >> 6;
		char next=1;
		if (n<0 || ctrl_state != ctrl) {
			next=0;
		}
		char ctrl_rr = CTRL_RR | ((ctrl_state+next)%2) << 7;

		if(create_frame(frame1, ctrl_rr))
		{
			send_frame(fd, frame1,NULL,0);
		}
	} while(ctrl_state != ctrl || n<0);

	ctrl_state = (ctrl_state+1)%2;

	return n;
}

int llclose(int fd) {

		int n;
		char from_address, ctrl, *buf;

		if (stop!=1 && (attempts < MAX_ATTEMPTS) ) {
			char frame1[MAX_FRAME]={0x7e};
			do{
				n=-1;
				if(create_frame(frame1,CTRL_DISC))
				{
					send_frame(fd, frame1,NULL,0);

					// Start timer
					if(flag){
						interrupt_alarm=0;
						alarm(3);	// activate timer of 3s
						flag=0;
					}
					while(n<0 && !flag)
					{
						n =	receive_frame(fd, &buf);
					}

					if(flag)	continue;
					flag=interrupt_alarm=1;
					n = read_frame(buf, n, NULL, &from_address, &ctrl);

					printf("%c",ctrl);

				}
			}
			while(ctrl!=CTRL_DISC);


			// Send final UA
			char frame2[MAX_FRAME];
			if(create_frame(frame2, CTRL_UA))
			{
				send_frame(fd, frame2, NULL, 0);
			}
		}

		sleep(1);

		if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);

	if (debugging)
		printf("Connection closed!\n");

	return 1;
}

int llwrite(int fd, char *data, int length){
	char ctrl_rr;
	int n=-1;
	char *buf;
	char from_address, ctrl;
	// Include a string in a frame and send it
	char frame1[MAX_FRAME]={0x7e};
	attempts=0;
	do{
		if(create_frame(frame1, ( ctrl_state << 6 )))
		{
			ctrl=-1;

			send_frame(fd, frame1,data,length);

			// Start timer
			if(flag){
				interrupt_alarm=0;

				alarm(3);	// activate timer of 3s
				flag=0;
			}

			do
			{
				n =	receive_frame(fd, &buf);
			}while(n<0 && !flag);

			if(flag)	continue;
			flag=interrupt_alarm=1;
			n = read_frame(buf, n, NULL, &from_address, &ctrl);
			
			ctrl_rr = CTRL_RR | ((ctrl_state+1)%2)<<7;
		}
	}
	while( (n < 0 || ctrl != ctrl_rr) && attempts < MAX_ATTEMPTS);
	if(attempts >= MAX_ATTEMPTS) return -1;
	ctrl_state = (ctrl_state+1)%2;

	return 1;
}
