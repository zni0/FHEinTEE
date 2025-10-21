#include <errno.h>
#include <getopt.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define SA struct sockaddr

#define EK_SIZE 2526028800lu
#define SK_SIZE 2000

char EK[EK_SIZE];
char SK[SK_SIZE];

enum FrameType {
	INIT,
	GEN,
	ENC,
	DEC,
	CINIT,
	NAND,
};

void send_frame_type(int connfd, int s_cmd) {
	int bytes = 0;
	char cmd[10];
	bzero(cmd, sizeof(cmd));
	switch (s_cmd) {
		case INIT:
			strcpy(cmd, "INITx");
			break;
		case GEN:
			strcpy(cmd, "GENxx");
			break;
		case ENC:
			strcpy(cmd, "ENCxx");
			break;
		case DEC:
			strcpy(cmd, "DECxx");
			break;
		case CINIT:
			strcpy(cmd, "CINIT");
			break;
		case NAND:
			strcpy(cmd, "NANDx");
			break;
		default:
			exit(1);
	}	

	bytes = write(connfd, cmd, sizeof(cmd));
        if(bytes != sizeof(cmd)) {
                printf("Error in sending frame type: %s\n", cmd);
                exit(1);
        }
}

void init_fhe(int connfd) {
	// Check if the enlcave is busy.
	unsigned long return_stat;
	int bytes = 0;

	send_frame_type(connfd, INIT);
	bytes = read(connfd, &return_stat, sizeof(unsigned long));
	if(bytes != sizeof(unsigned long)) {
		printf("init_fhe: Error\n");
		exit(1);
	}
	if(return_stat != 0) {
		printf("Enclave is busy\n");
		exit(1);
	}
}

void key_gen(int connfd) {
	// Send frame of type Gen Key and get back a secrect key and an eval Key
	unsigned long bytes = 0;
	send_frame_type(connfd, GEN);
	while(bytes != SK_SIZE)
		bytes += read(connfd, SK+bytes, SK_SIZE - bytes);
	bytes = 0;
	while(bytes != EK_SIZE)
		bytes += read(connfd, EK+bytes, EK_SIZE - bytes);
}

void enc(int connfd, int val, char * ret) {
	// Get an encryption of val
	unsigned long bytes = 0;
	if(val) val = 1;
	send_frame_type(connfd, ENC);
	bytes = write(connfd, &val, sizeof(int));
	if(bytes != sizeof(int)) {
                printf("Error in enc\n");
		exit(1);
	}
	bytes = 0;
	while(bytes != SK_SIZE)
		bytes += read(connfd, ret+bytes, SK_SIZE - bytes);
}

int dec(int connfd, char * val) {
	// Get a decryption of val
	unsigned long bytes = 0;
	int dec_val;
	
	send_frame_type(connfd, DEC);
	bytes = write(connfd, val, SK_SIZE);
	if(bytes != SK_SIZE) {
                printf("Error in dec\n");
		exit(1);
	}
	bytes = 0;
        bytes = read(connfd, &dec_val, sizeof(int));
        if(bytes != sizeof(int)) {
                printf("Error in dec\n");
                exit(1);
	}
	return dec_val;
}

unsigned long compute_init(int connfd) {
	// Send EK to comp server
	unsigned long  bytes = 0;
	unsigned long ret;
	
	send_frame_type(connfd, CINIT);
	bytes = write(connfd, EK, EK_SIZE);
	if(bytes != EK_SIZE) {
                printf("Error in compute_init\n");
		exit(1);
	}
	bytes = 0;
        bytes = read(connfd, &ret, sizeof(unsigned long));
        if(bytes != sizeof(unsigned long)) {
                printf("Error in compute_init\n");
                exit(1);
	}
	return ret;
}


void func(int connfd)
{
	char ret[SK_SIZE];
	key_gen(connfd);
	enc(connfd, 0, ret);
	printf("Return val is: %d\n", dec(connfd, ret));
}

int main(int argc, char **argv)
{
	if(argc != 2)  {
		printf("USAGE:\n");
		printf("./client <ip>");
		return 1;
	}
	int sockfd, connfd;
	struct sockaddr_in servaddr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);
	servaddr.sin_port = htons(PORT);
	while (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
		continue;
	}
	printf("Connected to the server");
	func(sockfd);
	close(sockfd);
}
