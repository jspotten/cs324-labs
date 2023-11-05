// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
// #define USERID 123456789

#define USERID 1823702874
#define BUFSIZE 8
#define SIZE 256

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
	//unsigned int port = atoi(argv[2]);
	unsigned int user_id = htonl(USERID);
	unsigned short level = atoi(argv[3]);
	unsigned short seed = htons(atoi(argv[4]));
	unsigned char buffer[BUFSIZE];
	
	bzero(buffer, BUFSIZE);
	memcpy(&buffer[1], &level, sizeof(unsigned short));
	memcpy(&buffer[2], &user_id, sizeof(unsigned int));
	memcpy(&buffer[6], &seed, sizeof(unsigned short));

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

	unsigned int *request;
	unsigned char receiver[SIZE];
	ssize_t nread = recvfrom(sfd, receiver, SIZE, 0, remote_addr, &addr_len);
	unsigned char chunk[1024];
	unsigned int chunk_len = (int)(receiver[0]);
	unsigned int total_bytes = 0;

	while(chunk_len != 0)
	{	
		memcpy(&chunk[total_bytes], &receiver[1], chunk_len);
		total_bytes += chunk_len;

		unsigned short opcode = receiver[chunk_len + 1];

		unsigned short opparam;
		unsigned int nonce;
		//printf("Hello\n");
		switch(opcode)
		{
			case 1:
				memcpy(&opparam, &receiver[chunk_len+2], 2);
				remote_addr_in.sin_port = opparam;
				remote_addr_in6.sin6_port = opparam;
				memcpy(&nonce, &receiver[chunk_len + 4], 4);
				nonce = htonl(ntohl(nonce) + 1);
				break;

			case 2:
				memcpy(&opparam, &receiver[chunk_len+2], 2);
				local_addr_in.sin_family = AF_INET;
				local_addr_in.sin_port = opparam;
				local_addr_in.sin_addr.s_addr = 0;

				local_addr_in6.sin6_family = AF_INET6;
				local_addr_in6.sin6_port = opparam;
				bzero(local_addr_in6.sin6_addr.s6_addr, 16);

				close(sfd);
				sfd = socket(local_addr_in.sin_family, result->ai_socktype, result->ai_protocol);
				local_addr = (struct sockaddr *)&local_addr_in;
				if (bind(sfd, local_addr, addr_len) < 0) {
					perror("bind()");
				}
				memcpy(&nonce, &receiver[chunk_len + 4], 4);
				nonce = htonl(ntohl(nonce) + 1);
				break;
			case 3:
				unsigned short m;
				memcpy(&m, &receiver[chunk_len+2], 2);
				m = htons(m);
				unsigned int sum = 1;
				struct sockaddr_in temp_remote;
				struct sockaddr *temp_remote_addr = (struct sockaddr *)&temp_remote;
				printf("%d\n", m);
				for(int i = 0; i < m; i++)
				{
					nread = recvfrom(sfd, receiver, 0, 0, temp_remote_addr, &addr_len);
					sum += temp_remote.sin_port;
					printf("%d\n", temp_remote.sin_port);
				}
				printf("%d\n", sum);
				sum = ntohl(sum);
				printf("%d\n", sum);
				memcpy(&nonce, &sum, 4);
				break;

			default:
				memcpy(&nonce, &receiver[chunk_len + 4], 4);
				nonce = htonl(ntohl(nonce) + 1);
				break;
		}

		memcpy(&request, &nonce, 4);
		sendto(sfd, &request, 4, 0, remote_addr, addr_len);
		
		nread = recvfrom(sfd, receiver, SIZE, 0, remote_addr, &addr_len);
		chunk_len = (int)(receiver[0]);
	}
	chunk[total_bytes] = '\0';
	printf("%s\n", &chunk);
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
