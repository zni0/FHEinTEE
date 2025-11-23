#include "utils.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define PORT 8081
#define SK_SIZE (sizeof(uint64_t) * 2)
#define EK_SIZE (sizeof(uint64_t) * 2)

enum FrameType {GEN, ENC, DEC};

static uint64_t n, g, lambda, mu;
static uint64_t p = 13, q = 17;

static void recv_exact(int fd, void *buf, size_t len)
{
	size_t received = 0;
	while (received < len) {
		int r = recv(fd, (char*)buf + received, len - received, 0);
		if (r < 0) sgx_exit(NULL);
		received += r;
	}
}

uint64_t paillier_encrypt(uint64_t m, uint64_t n, uint64_t g)
{
	uint64_t n2 = n * n;
	uint64_t r = (rand() % (n - 2)) + 2;

	if (gcd64(n, r) != 1)
		sgx_exit(NULL);

	uint64_t gm = pow_mod64(g, m, n2);
	uint64_t rn = pow_mod64(r, n, n2);
	return mul_mod64(gm, rn, n2);
}

uint64_t paillier_decrypt(uint64_t c, uint64_t n, uint64_t lambda)
{
	uint64_t n2 = n * n;
	uint64_t u = pow_mod64(c, lambda, n2);
	uint64_t L = (u - 1) / n;
	return mul_mod64(L, mu, n);
}

static void handle_gen(int clnt_fd)
{
	uint64_t SKbuf[2];
	uint64_t EKbuf[2];
	srand(12345);
	n = p * q;
	uint64_t phi = (p - 1) * (q - 1);
	if (gcd64(n, phi) != 1) {
		puts("Invalid primes: gcd(pq, (p-1)(q-1)) != 1");
		sgx_exit(NULL);
	}

	lambda = lcm64(p - 1, q - 1);

	uint64_t n2 = n * n;
	g = (rand() % (n2 - 2)) + 2;
	while (gcd64(n2, g) != 1) {
		g = (rand() % (n2 - 2)) + 2;
		puts("Invalid g: gcd(g, n2) != 1");
	}

	uint64_t u = pow_mod64(g, lambda, n2);
	uint64_t L = (u - 1) / n;

	if (!modinv(L % n, n, &mu))
		sgx_exit(NULL);

	SKbuf[0] = lambda;
	SKbuf[1] = mu;

	EKbuf[0] = n;
	EKbuf[1] = g;

	send(clnt_fd, SKbuf, SK_SIZE, 0);
	send(clnt_fd, EKbuf, EK_SIZE, 0);
}

static void handle_enc(int clnt_fd)
{
	int val;
	recv_exact(clnt_fd, &val, sizeof(int));
	uint64_t c = paillier_encrypt(val, n, g);
	send(clnt_fd, &c, sizeof(uint64_t), 0);
}

static void handle_dec(int clnt_fd)
{
	uint64_t c;
	recv_exact(clnt_fd, &c, sizeof(uint64_t));
	int m = (int)paillier_decrypt(c, n, lambda);
	send(clnt_fd, &m, sizeof(int), 0);
}

void enclave_main()
{
	int srvr_fd;
	int clnt_fd;
	char cmd[10];
	struct sockaddr_in addr;

	srvr_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (srvr_fd == -1) {
		puts("Error in getting srvr_fd.");
		sgx_exit(NULL);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(srvr_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
		sgx_exit(NULL);

	if (listen(srvr_fd, 10) != 0)
		sgx_exit(NULL);

	puts("Enclave server started.");
	while (1) {
		struct sockaddr_in cli;
		socklen_t len = sizeof(cli);

		clnt_fd = accept(srvr_fd, (struct sockaddr *)&cli, &len);
		if (clnt_fd < 0) {
			puts("ERROR on accept");
			continue;
		}

		while (1) {
			memset(cmd, 0, sizeof(cmd));
			recv_exact(clnt_fd, cmd, sizeof(cmd));
			if (!strncmp(cmd, "GEN", 3))
				handle_gen(clnt_fd);
			else if (!strncmp(cmd, "ENC", 3))
				handle_enc(clnt_fd);
			else if (!strncmp(cmd, "DEC", 3))
				handle_dec(clnt_fd);
			else {
				break;
			}
		}
		close(clnt_fd);
	}
	puts("Exiting...");
	close(srvr_fd);
	sgx_exit(NULL);
}
