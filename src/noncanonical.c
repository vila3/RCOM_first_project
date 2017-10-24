/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>

#include "rs232.h"

#define PACK_HEAD_LEN 4
#define PACK_NET_LEN PAYLOAD-PACK_HEAD_LEN

#define PACK_DATA 1
#define PACK_START 2
#define PACK_END 3

#define PACK_T_SIZE 0
#define PACK_T_NAME 1

int read_package_ctr_size(char *pack, int pack_len) {
	int i, j, file_size=0;
	for (i = 1; i < pack_len; i++) {
		if(pack[i] == PACK_T_SIZE) {
			for (j = pack[i+1]; j > 0; j--) {
				file_size |= (pack[i+1+j]&0xff)<<(pack[i+1] - j)*8;
			}
			return file_size;
		}
		i++;
		i += pack[i];
	}

	return -1;
}

int read_package_ctr_name(char *pack, int pack_len, char **name) {
	int i, j;
	char *new_name;
	for (i = 1; i < pack_len; i++) {
		if(pack[i] == PACK_T_NAME) {
			new_name = malloc(pack[i+1]);
			for (j = 0; j < pack[i+1]; j++) {
				new_name[j] = pack[i+2+j];
			}
			*name = new_name;
			return pack[i+1];
		}
		i++;
		i += pack[i];
	}

	return -1;
}

int read_package_data(char *pack, char **data, int *seq) {
	int i, pack_size=0;;

	*seq = pack[1];

	pack_size = (pack[2]<<8) | (pack[3]&0xff);
	*data = malloc(pack_size);
	for (i = 0; i < pack_size; i++) {
		(*data)[i] = pack[4+i];
	}

	return pack_size;
}

int main(int argc, char** argv)
{

	/**
	 * Test read functions
	 */

	// char test[10] = {0x02, 0x01, 0x03, 'a', 'b', '\0', 0x00, 0x02, 0x02, 0x02};
	// int file_size = read_package_ctr_size(test, 10);
	// char *name;
	// int name_size = read_package_ctr_name(test, 10, &name);
	//
	// printf("size: %d\n", file_size);
	// printf("name: %s\n", name);

	int port, n, fd, file_size, file_arr_init=0, bytes_read=0, pack_data_size=0, seq, i;
	char *file_arr, *pack_data;

	port=llopen(argv[1], RECEIVER);


	char data[MAX_FRAME];
	fd = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IXUSR);

	do {
		n = llread(port,data);
		if (n<=0) break;

		// print_frame(data, n);

	  	if (data[0]==PACK_START || data[0]==PACK_END) {

		  	file_size = read_package_ctr_size(data, n);
			// name_size = read_package_ctr_name(pack, n, &name);
			// printf("name: %s\n", name);
			printf("file size: %d\n", file_size);
			if (!file_arr_init) {
				file_arr = malloc(file_size);
				file_arr_init = 1;
			}

		} else {
			if (file_arr_init) {
				pack_data_size = read_package_data(data, &pack_data, &seq);
				write(fd, pack_data, pack_data_size);
				// memcpy(file_arr+bytes_read, pack_data, pack_data_size);
				free(pack_data);
				pack_data = NULL;
				bytes_read+=pack_data_size;
			}
		}
    } while(1);

	close(fd);

    llclose(port);

    return 0;
}
