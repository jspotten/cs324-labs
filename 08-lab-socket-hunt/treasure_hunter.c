// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
// #define USERID 123456789

#define USERID 1823702874
#define BUFSIZE 8

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

int verbose = 0;

void print_bytes(unsigned char *bytes, int byteslen);

int main(int argc, char *argv[]) 
{
	char *server = argv[1];
	char *char_port = argv[2];
	unsigned int port = atoi(argv[2]);
	unsigned int user_id = htonl(USERID);
	unsigned short level = atoi(argv[3]);
	unsigned short seed = htons(atoi(argv[4]));
	unsigned char buffer[BUFSIZE];
	
	bzero(buffer, BUFSIZE);
	memcpy(&buffer[1], &level, sizeof(unsigned short));
	memcpy(&buffer[2], &user_id, sizeof(unsigned int));
	memcpy(&buffer[6], &seed, sizeof(unsigned short));
	print_bytes(buffer, 8);

	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	struct addrinfo *result;
	getaddrinfo(server, char_port, &hints, &result);
	int sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	int addr_fam;
	socklen_t addr_len = result->ai_addrlen;

	struct sockaddr_in remote_addr_in;
	struct sockaddr_in6 remote_addr_in6;
	struct sockaddr *remote_addr;
	unsigned short remote_port;

	struct sockaddr_in local_addr_in;
	struct sockaddr_in6 local_addr_in6;
	struct sockaddr *local_addr;

	remote_addr_in = *(struct sockaddr_in *)result->ai_addr;
	remote_port = ntohs(remote_addr_in.sin_port);
	remote_addr = (struct sockaddr *)&remote_addr_in;
	local_addr = (struct sockaddr *)&local_addr_in;
	
	if(sendto(sfd, buffer, BUFSIZE, 0, remote_addr, addr_len) != BUFSIZE)
	{
		fprintf(stderr, "partial/failed write\n");
		exit(EXIT_FAILURE);
	}
	unsigned char buffer2[256];
	ssize_t nread = recvfrom(sfd, buffer2, 256, 0, remote_addr, &addr_len);
	print_bytes(buffer2, nread);
}


void print_bytes(unsigned char *bytes, int byteslen) {
	int i, j, byteslen_adjusted;

	if (byteslen % 8) {
		byteslen_adjusted = ((byteslen / 8) + 1) * 8;
	} else {
		byteslen_adjusted = byteslen;
	}
	for (i = 0; i < byteslen_adjusted + 1; i++) {
		if (!(i % 8)) {
			if (i > 0) {
				for (j = i - 8; j < i; j++) {
					if (j >= byteslen_adjusted) {
						printf("  ");
					} else if (j >= byteslen) {
						printf("  ");
					} else if (bytes[j] >= '!' && bytes[j] <= '~') {
						printf(" %c", bytes[j]);
					} else {
						printf(" .");
					}
				}
			}
			if (i < byteslen_adjusted) {
				printf("\n%02X: ", i);
			}
		} else if (!(i % 4)) {
			printf(" ");
		}
		if (i >= byteslen_adjusted) {
			continue;
		} else if (i >= byteslen) {
			printf("   ");
		} else {
			printf("%02X ", bytes[i]);
		}
	}
	printf("\n");
}
