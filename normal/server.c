#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

static uint64_t n, g, lambda, mu;
static uint64_t p = 13, q = 17;

uint64_t mul_mod64(uint64_t a, uint64_t b, uint64_t mod) {
	uint64_t res = 0;
	a %= mod;

	while (b > 0) {
		if (b & 1)
			res = (res + a) % mod;
		a = (a * 2) % mod;
		b >>= 1;
	}
	return res;
}

int recv_exact(int fd, void *buf, size_t len)
{
	size_t received = 0;
	while (received < len) {
		int r = recv(fd, (char*)buf + received, len - received, 0);
		if(r==0) return 0;
		if (r < 0) exit(1);
		received += r;
	}
	return received;
}

int main() {
	int srvr_fd, connfd;
	struct sockaddr_in servaddr, cli;
	char cmd[10];

	n = p*q;
	srvr_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (srvr_fd == -1) { perror("socket"); exit(1); }

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	if (bind(srvr_fd, (struct sockaddr*)&servaddr, sizeof(servaddr))) {
		perror("bind");
		exit(1);
	}
	if (listen(srvr_fd, 5)) {
		perror("listen");
		exit(1);
	}
	printf("Server started on port %d\n", PORT);
	socklen_t len = sizeof(cli);
	while (1) {
		struct sockaddr_in cli;
		socklen_t len = sizeof(cli);

		connfd = accept(srvr_fd, (struct sockaddr *)&cli, &len);
		if (connfd < 0) {
			printf("ERROR on accept\n");
			continue;
		}

		while (1) {
			memset(cmd, 0, sizeof(cmd));
			recv_exact(connfd, cmd, sizeof(cmd));
			if (!strncmp(cmd, "ADD", 3)) {
				uint64_t in1, in2;
				uint64_t n2 = n * n;
				recv_exact(connfd, &in1, sizeof(uint64_t));
				recv_exact(connfd, &in2, sizeof(uint64_t));
				uint64_t out = mul_mod64(in1, in2, n2);
				send(connfd, &out, sizeof(uint64_t), 0);
				printf("Need to ADD %llu, %llu\n", in1, in1);
			}
			else {
				break;
			}
		}
		close(connfd);
	}
	close(srvr_fd);
	return 0;
}
