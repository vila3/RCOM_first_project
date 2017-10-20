
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

#define PACK_HEAD_LEN 4
#define PACK_NET_LEN PAYLOAD-PACK_HEAD_LEN

int create_packages(char *frame, char *data, int data_size, int *package_len){
	int i, w=4;
	// start
		frame[w++]=0x02; // Control field (2 - start)

		// Type, length and Value
		frame[w++]=0x00; // T1=0 - File size
		frame[w++]=sizeof(data_size); // L1=2 - Length of V (next field)
		frame[w++]=data_size;	   // V1

	// insert
		frame[w++]=0x01; // Control field (1 - data)
		frame[w++]=0;//TODO sequence_number; // N - Sequence (N)umber
		frame[w++]=0; // L2 - TODO
		frame[w++]=0; // L1 - TODO
		// loop por insert
		for(i=0; (i<(PAYLOAD-w)) && (i<data_size) ;i++){
			frame[w++]=data[i];
		}

	// end
		frame[w++]=0x03; // Control field (3 - end)

		if(i==data_size)
			return 1; // needs more frames to send data
		else if(i<data_size)
			return 0; // all data included in package
		else
			return -1;
}

int main(int argc, char** argv)
{
	int fd;
	ssize_t bytes_read;
	char *buffer;
	int debugging=0;
	//int i;
	int file_byte_size, bytes_left, read_size, total_read;
	// read port
	if ( (argc < 2) ||
  	     (strstr(argv[1], "/dev/") == NULL)) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

    llopen(argv[1], TRANSMITTER);

	// read file do transmit

	buffer = (char*) malloc( sizeof(char) * PAYLOAD);
	fd = open(argv[2],O_RDONLY);
	if(fd){
		if(debugging)
			printf("Ficheiro aberto!\n");

			bytes_left=file_byte_size=lseek(fd,0,SEEK_END);
		do{
			read_size=(bytes_left>PACK_NET_LEN)?PACK_NET_LEN:bytes_left;
			bytes_read=read(fd,buffer,read_size);
			bytes_left-=bytes_read;

			total_read+=bytes_read;
		}
		while(bytes_left>0);
		printf("Número de bytes lidos: %d\n",total_read); //10968
	}

	/*for(i=0; i<total_read; i++){
		printf("%c",buffer[i]);
	}*/

	// close file to transmit
	/*
	fclose(file);
	if(debugging)
		printf("Ficheiro fechado!\n");
	*/

	printf("Send string: \"String to ~ send!\"\n");
	//sleep(4000);
	llwrite(fd,"String to ~ send!\0",18);
	llwrite(fd,"String ot ~ send!\0",18);
	//llwrite(fd,buffer,total_read);

	llclose();

	return 0;
}
