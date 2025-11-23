#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8081
#define SA struct sockaddr

#define SK_SIZE (sizeof(uint64_t) * 2)   // lambda, mu
#define EK_SIZE (sizeof(uint64_t) * 2)   // n, g

enum FrameType { GEN, ENC, DEC, ADD };
void send_cmd(int connfd, enum FrameType t) {
    char cmd[10];
    memset(cmd, 0, sizeof(cmd));
    switch (t) {
        case GEN: strcpy(cmd, "GENxx"); break;
        case ENC: strcpy(cmd, "ENCxx"); break;
        case DEC: strcpy(cmd, "DECxx"); break;
        case ADD: strcpy(cmd, "ADDxx"); break;
        default: exit(1);
    }

    if (write(connfd, cmd, sizeof(cmd)) != sizeof(cmd)) {
        perror("write cmd");
        exit(1);
    }
}

void read_exact(int fd, void *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t r = read(fd, buf + got, len - got);
        if (r <= 0) {
            perror("read");
            exit(1);
        }
        got += r;
    }
}

void generate_keys(int connfd,
                   uint64_t *lambda, uint64_t *mu,
                   uint64_t *n, uint64_t *g)
{
    send_cmd(connfd, GEN);

    uint64_t SKbuf[2];
    uint64_t EKbuf[2];

    read_exact(connfd, SKbuf, SK_SIZE);
    read_exact(connfd, EKbuf, EK_SIZE);

    *lambda = SKbuf[0];
    *mu     = SKbuf[1];
    *n      = EKbuf[0];
    *g      = EKbuf[1];

/*
    printf("Keys received:\n");
    printf("  lambda = %llu\n", (unsigned long long)*lambda);
    printf("  mu     = %llu\n", (unsigned long long)*mu);
    printf("  n      = %llu\n", (unsigned long long)*n);
    printf("  g      = %llu\n", (unsigned long long)*g);
*/
}

uint64_t enc_val(int connfd, int x) {
    send_cmd(connfd, ENC);

    if (write(connfd, &x, sizeof(int)) != sizeof(int)) {
        perror("write enc");
        exit(1);
    }

    uint64_t c;
    read_exact(connfd, &c, sizeof(uint64_t));
    return c;
}

int dec_val(int connfd, uint64_t c) {
    send_cmd(connfd, DEC);

    if (write(connfd, &c, sizeof(uint64_t)) != sizeof(uint64_t)) {
        perror("write dec");
        exit(1);
    }

    int m;
    read_exact(connfd, &m, sizeof(int));
    return m;
}

uint64_t add_ciphertexts(uint64_t c1, uint64_t c2) {
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) { perror("socket"); exit(1); }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(8080);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Connecting to untrusted server...\n");
    while (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0) {
        continue;
    }
    printf("...Connected...\n");

    send_cmd(sockfd, ADD);

    if (write(sockfd, &c1, sizeof(uint64_t)) != sizeof(uint64_t)) {
        perror("write and");
        exit(1);
    }
    if (write(sockfd, &c2, sizeof(uint64_t)) != sizeof(uint64_t)) {
        perror("write and");
        exit(1);
    }

    uint64_t m;
    read_exact(sockfd, &m, sizeof(uint64_t));
    return m;
}

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) { perror("socket"); exit(1); }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Connecting with sgx enclave...\n");
    while (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0) {
        continue;
    }
    printf("...Connected...\n\n");

    uint64_t lambda, mu, n, g;

    generate_keys(sockfd, &lambda, &mu, &n, &g);

    int a, b;
    if(argc == 3) {
	    a = atoi(argv[1]);
	    b = atoi(argv[2]);
    } else{
	    printf("Enter first integer: ");
	    scanf("%d", &a);
	    printf("Enter second integer: ");
	    scanf("%d", &b);
    }
    uint64_t enc1 = enc_val(sockfd, a);
    uint64_t enc2 = enc_val(sockfd, b);

    printf("Encrypted a = %llu\n", (unsigned long long)enc1);
    printf("Encrypted b = %llu\n", (unsigned long long)enc2);

    uint64_t enc_sum = add_ciphertexts(enc1, enc2);

    int result = dec_val(sockfd, enc_sum);
    printf("Decrypted (a + b) = %d\n", result);

    close(sockfd);
    if((a+b)%n != result) {
	    printf("ERROR, wrong output\n");
	    exit(1);
    }
    return 0;
}
