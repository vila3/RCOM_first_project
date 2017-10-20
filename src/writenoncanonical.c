
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
	//FILE *file;
	//size_t cadence=1, size=1, read=0, total_read=0;
	//char *buffer;
	//int debugging=0;
	//int i;

	// read port
	if ( (argc < 2) ||
  	     (strstr(argv[1], "/dev/") == NULL)) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

    llopen(argv[1], TRANSMITTER);

	// read file do transmit
	/*
	buffer = (char*) malloc( sizeof(char) * MAX_BUFFER_FILE);
	file = fopen("./penguin.gif","r");
	if(file){
		if(debugging)
			printf("Ficheiro aberto!\n");
		do{
			read=fread(buffer,size,cadence,file);
			total_read+=read;
		}
		while(read>0);
		printf("Número de bytes lidos: %zu\n",total_read);
	}*/

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
