
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

int main(int argc, char** argv)
{
	FILE *file;
	size_t cadence=1, size=1, read=0, total_read=0;
	char *buffer;
	int debugging=0;
	//int i;

	// read port
	if ( (argc < 2) ||
  	     (strstr(argv[1], "/dev/") == NULL)) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

    llopen(argv[1], TRANSMITTER);

	// read file do transmit
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
	}

	/*for(i=0; i<total_read; i++){
		printf("%c",buffer[i]);
	}*/
	
	// close file to transmit
	fclose(file);
	if(debugging)
		printf("Ficheiro fechado!\n");


	//printf("Send string: \"String to ~ send!\"\n");
	//sleep(4000);
	llwrite(fd,buffer,total_read);

	llclose();

	return 0;
}
