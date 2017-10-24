/*Non-Canonical Input Processing*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include "rs232.h"

#define PACK_HEAD_LEN 4
#define PACK_NET_LEN PAYLOAD-PACK_HEAD_LEN
#define PACK_START 2
#define PACK_DATA 1
#define PACK_END 3

int create_packages(char *buffer, char **plbuffer, int data_size, char ctrl, int filesize, char *sequence_number, char *filename){
	int i, w=0;
	*plbuffer = (char*) malloc(sizeof(char)*PAYLOAD);

	(*plbuffer)[w++]=ctrl; // Control field
	//printf("1\n");
	if(ctrl==PACK_START || ctrl==PACK_END){
		//printf("2\n");
		// Type, length and Value
		(*plbuffer)[w++]=0x00; // T1=0 - File size
		//printf("3\n");
		(*plbuffer)[w++]=(char)sizeof(int); // L1=2 - Length of V (next field)

		for(i=0; i < sizeof(int); i++){
			(*plbuffer)[w++]=0xff&(filesize>>( (sizeof(int)-1-i) *8));
		}

		// Type, length and Value
		(*plbuffer)[w++]=0x01; // T1=0 - File size
		(*plbuffer)[w++]=(char)strlen(filename); // L1=2 - Length of V (next field)

		for(i=0; i < strlen(filename); i++){
			(*plbuffer)[w++]=filename[i];
		}

	} else{
		// insert
		(*plbuffer)[w++]=*sequence_number;// N - Sequence (N)umber
		(*plbuffer)[w++]=(data_size>>8)&0xff; // L2 - MSB
		(*plbuffer)[w++]=data_size&0xff; // L1 - LSB
		// loop for insert
		for(i=w; i < PACK_NET_LEN+w; i++){
			(*plbuffer)[i]=(buffer[i-w]);
		}
		w=i;
		(*sequence_number)++;
	}
	return w;
}

int main(int argc, char** argv)
{
	char *filename;
	int port,fd;
	ssize_t bytes_read=0, bytes_write;
	char *buffer,*plbuffer;
	int debugging=0;
	//int i;
	int file_byte_size, bytes_left, read_size, total_read=0, total_write=0, pack_size, name_size;
	char sequence_number=0;
	// read port
	if ( (argc < 2) ||
		     (strstr(argv[1], "/dev/") == NULL)) {
	  printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
	  exit(1);
	}

	port = llopen(argv[1], TRANSMITTER);

	// read file do transmit
	buffer = (char*) malloc( sizeof(char) * PACK_NET_LEN);
	fd = open(argv[2],O_RDONLY);
	filename=basename(argv[2]);
	if(fd){
		if(debugging)
			printf("Ficheiro aberto!\n");

		bytes_left=file_byte_size=lseek(fd,0,SEEK_END);
		lseek(fd,0,SEEK_SET);

		pack_size=create_packages(buffer,&plbuffer,bytes_read,PACK_START,file_byte_size,&sequence_number,filename);
		llwrite(port,plbuffer,pack_size);
		do{
			lseek(fd,file_byte_size-bytes_left,SEEK_SET);
			read_size=(bytes_left>PACK_NET_LEN)?PACK_NET_LEN:bytes_left;

			bytes_read=read(fd,buffer,read_size);

			// Test PL
			pack_size=create_packages(buffer,&plbuffer,bytes_read,PACK_DATA,file_byte_size,&sequence_number,NULL);
			//

			bytes_write=llwrite(port,plbuffer,pack_size);
			bytes_left-=bytes_read;

			total_read+=bytes_read;
			total_write+=bytes_write;
		}
		while(bytes_left>0 && bytes_write>=0);

		if(bytes_left > 0){
			printf("Transfer fail due to MAX_ATTEMPTS (%d)\n",MAX_ATTEMPTS);
			pack_size=create_packages(buffer,&plbuffer,bytes_read,PACK_END,file_byte_size,&sequence_number,filename);
			llwrite(port,plbuffer,pack_size);
		}

		// printf("Número de bytes lidos: %d\n",total_read); //10968
		// printf("Número de bytes escritos: %d\n",total_write); //10968
	}

	llclose(port);

	return 0;
}
