#include <iostream>
#include <cstdlib>
#include "../LWE.h"
#include "../FHEW.h"
#include "common.h"
#include <cassert>
#define PORT 8080

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define SA struct sockaddr

#define EK_SIZE 2526028800lu
#define SK_SIZE 2000
#define V_SIZE 2000
char * EK;
char SK[V_SIZE];


enum FrameType {
	CINIT,
	CNAND,

	// Split into enclave
	GEN,
	ENC,
	DEC,
};

void handle_cinit(int connfd) {
	printf("In handle_cinit\n");
	int bytes = 0;
	while(bytes != EK_SIZE)
		bytes += read(connfd, EK+bytes, EK_SIZE - bytes);
}

void handle_nand(int connfd) {
	printf("In handle_nand\n");
	int bytes = 0;
	char v1[V_SIZE], v2[V_SIZE], v3[V_SIZE];
        while(bytes != V_SIZE)
                bytes += read(connfd, v1+bytes, V_SIZE - bytes);
	bytes = 0;
        while(bytes != V_SIZE)
                bytes += read(connfd, v2+bytes, V_SIZE - bytes);

	// Do computation on v1, v2 and get v3
	bytes = write(connfd, v3, V_SIZE);
	if(bytes != V_SIZE) {
		printf("Error in dec\n");
		exit(1);
	}
}


// Split into enclave
void handle_gen(int connfd) {
	size_t total_sent = 0;
	printf("In handle_gen\n");
	FHEW::EvalKey EKey;
	LWE::SecretKey LWEsk;
	// srand(time(NULL));
	FHEW::Setup();
	LWE::KeyGen(LWEsk);
	FHEW::KeyGen(&EKey, LWEsk);
	SaveEvalKey(&EKey, EK);
	SaveSecretKey(&LWEsk,SK);
	printf("KeyGen complete\n");
	// Send SK
	while (total_sent < SK_SIZE) {
		size_t bytes = write(connfd, SK + total_sent, SK_SIZE - total_sent);
		if (bytes == 0) {
			printf("Error in sending SK\n");
			exit(1);
		}
		total_sent += bytes;
	}
	printf("SK sent\n");
	// Send EK
	total_sent = 0;
	while (total_sent < EK_SIZE) {
		size_t bytes = write(connfd, EK + total_sent, EK_SIZE - total_sent);
		if (bytes == 0) {
			printf("Error in sending EK\n");
			exit(1);
		}
		total_sent += bytes;
	}
	printf("EK sent\n");
}

void handle_enc(int connfd) {
	printf("In handle_enc\n");
	int message;
	int bytes = 0;
	bytes = read(connfd, &message, sizeof(int));
	if(bytes != sizeof(int)) {
		printf("handle_enc: Error\n");
		exit(1);
	}
	printf("handle_enc message is: %d\n", message);
	LWE::SecretKey* SKey = LoadSecretKey(SK);
	LWE::CipherText ct;
	LWE::Encrypt(&ct, *SKey, message);
	// Send cipher text to client
	//  SaveCipherText(&ct,ct_fn);
	size_t total_sent = 0;
	char temp[sizeof(LWE::CipherText)];
	memcpy(temp, &ct, sizeof(LWE::CipherText));
	while (total_sent < SK_SIZE) {
		size_t bytes = write(connfd, temp + total_sent, SK_SIZE - total_sent);
		if (bytes == 0) {
			printf("Error in sending ENC data\n");
			exit(1);
		}
		total_sent += bytes;
	}
}
void handle_dec(int connfd) {
	printf("In handle_dec\n");
	char V1[SK_SIZE];
	unsigned long bytes = 0;
	while(bytes != SK_SIZE)
		bytes += read(connfd, V1+bytes, SK_SIZE - bytes);
	printf("Read enc val\n");

	LWE::SecretKey* SKey = LoadSecretKey(SK);
	// LWE::CipherText* ct = LoadCipherText(ct_fn);
	LWE::CipherText* ct = new LWE::CipherText;
	memcpy(ct, V1, sizeof(LWE::CipherText));	
	// LWE::Encrypt(&ct, *SKey, message);
	// Send cipher text to client
	//  SaveCipherText(&ct,ct_fn);
	// size_t total_sent = 0;
	// char temp[sizeof(LWE::CipherText)];
	int m = LWE::Decrypt(*SKey,*ct);
	bytes = write(connfd, &m, sizeof(int));
	if(bytes != sizeof(int)) {
                printf("Error in dev\n");
		exit(1);
	}
}

int get_frame_type(char * buff) {
	if(strncmp("CINIT", buff, 5) == 0) return CINIT;
	if(strncmp("CNAND", buff, 5) == 0) return CNAND;
	// Split into enclave
	if(strncmp("GENxx", buff, 5) == 0) return GEN;
        if(strncmp("ENCxx", buff, 5) == 0) return ENC;
        if(strncmp("DECxx", buff, 5) == 0) return DEC;
	return -1;
}

#define PORT 8080

void accept_connection(int connfd) {

	char buff[10];

	while(1) {
		printf("In accept connection while\n");
		int bytes_read = 0;
		bzero(buff, sizeof(buff));

		// read the frame type from client and copy it in buffer
		// The client will always send first 10 chars as the frame type
		bytes_read = read(connfd, buff, sizeof(buff));
		printf("In accept connection bytes_read: %d\n", bytes_read);

		if(bytes_read<0) {
			break;
			// exit(1);
		}
		if(bytes_read == 0) {
			// Client has closed the connection
			break;
		}
		if(bytes_read != 10) {
			// TODO: Should continue reading instead of failing...
			exit(1);
		}
		printf("Got frame type: %s\n", buff);

		// Figure out the frame type
		switch (get_frame_type(buff)) {
			case CINIT:
				handle_cinit(connfd);
				break;
			case CNAND:
				handle_nand(connfd);
				break;
        		// Split into enclave
			case GEN:
				handle_gen(connfd);
				break;
			case ENC:
				handle_enc(connfd);
				break;
			case DEC:
				handle_dec(connfd);
				break;
			default:
				exit(1);
		}
	}
}

int main() {
	int sockfd, connfd;
	struct sockaddr_in servaddr, cli;

	EK = (char *) malloc(2526028800lu);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(1);
	}
	else
		printf("Socket successfully created..\n");
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // htonl(INADDR_LOOPBACK); // ("127.0.0.1"); // htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
		printf("socket bind failed...\n");
		exit(1);
	}
	printf("Socket successfully binded..\n");

	if ((listen(sockfd, 5)) != 0) {
		printf("Listen failed...\n");
		exit(1);
	}
	printf("Server listening..\n");
	while(1) {
		printf("server waiting to accept the client...\n");
		socklen_t len = sizeof(cli);
		connfd = accept(sockfd, (struct sockaddr*)&cli, &len);
		if (connfd < 0) {
			printf("server accept failed...\n");
			exit(1);
		}
		printf("server accept the client...\n");
		accept_connection(connfd);
		printf("Client closed\n");
	}
	close(sockfd);
	return 0;
}
