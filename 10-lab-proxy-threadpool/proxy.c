#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <asm-generic/socket.h>

/* Recommended max object size */
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

int open_sfd(char *, struct sockaddr*, socklen_t);
void handle_client(int, struct sockaddr*, socklen_t);
int complete_request_received(char *);
int parse_request(char *, char *, char *, char *, char *);
void test_parser();
void print_bytes(unsigned char *, int);


int main(int argc, char *argv[])
{
	char *port = argv[1];
	struct sockaddr_in ipv4addr;
	struct sockaddr *local_addr;

	ipv4addr.sin_family = AF_INET;
	ipv4addr.sin_addr.s_addr = INADDR_ANY;
	ipv4addr.sin_port = htons(atoi(port)); 
	socklen_t addr_len = sizeof(ipv4addr);
	local_addr = (struct sockaddr *)&ipv4addr;

	int sfd = open_sfd(port, local_addr, addr_len);
	while(1)
	{
		int fd = accept(sfd, local_addr, &addr_len);
		handle_client(fd, local_addr, addr_len);
	}
	return 0;
}


int open_sfd(char *port, struct sockaddr *local_addr, socklen_t addr_len)
{
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	int optval = 1;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
	
	bind(sfd, local_addr, addr_len);
	listen(sfd, 500);
	return sfd;
}


void handle_client(int fd, struct sockaddr *local_addr, socklen_t addr_len)
{
	unsigned char request[MAX_OBJECT_SIZE];
	int nread = 0;
	int bytes_read = 0;
	char method[16], hostname[64], port[8], path[64];
	for(;;)
	{		
		nread = recv(fd, request + bytes_read, MAX_OBJECT_SIZE, 0);
		bytes_read += nread;
		if(complete_request_received((char*)request) == 0)
			continue;
		else if(complete_request_received((char*)request) == 1)
		{
			break;
		}
	}
	request[bytes_read] = '\0';
	parse_request((char*)request, method, hostname, port, path);
	
	unsigned char response[MAX_OBJECT_SIZE];
	if(strcmp("80", port) == 0)
	{
		sprintf((char*)response, 
				"%s %s HTTP/1.0\r\nHost: %s\r\n%s\r\nConnection: Keep-Alive\r\nProxy-Connection: Keep-Alive\r\n\r\n", 
				method,
				path, 
				hostname,
				user_agent_hdr);
	}
	else
	{
		sprintf((char*)response, 
				"%s %s HTTP/1.0\r\nHost: %s:%s\r\n%s\r\nConnection: Keep-Alive\r\nProxy-Connection: Keep-Alive\r\n\r\n", 
				method,
				path, 
				hostname,
				port,
				user_agent_hdr);
	}

	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	struct addrinfo *result;
	int getaddr_res = getaddrinfo(hostname, port, &hints, &result);
	if(getaddr_res != 0)
		printf("Error getaddr: %d\n", getaddr_res);
	int sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	addr_len = result->ai_addrlen;
	struct sockaddr_in remote_addr_in;
	struct sockaddr *remote_addr;
	remote_addr_in = *(struct sockaddr_in *)result->ai_addr;
	remote_addr = (struct sockaddr *)&remote_addr_in;

	if(connect(sfd, remote_addr, addr_len) == -1)
	{
		printf("Error connecting\n");
	}
	send(sfd, response, strlen((char*)response), 0);

	bzero(request, MAX_OBJECT_SIZE);
	nread = 0;
	bytes_read = 0;
	for(;;)
	{		
		nread = recv(sfd, request + bytes_read, MAX_OBJECT_SIZE, 0);
		bytes_read += nread;
		if(nread == -1)
			continue;
		else if(nread == 0)
		{
			break;
		}
	}
	request[bytes_read] = '\0';
	print_bytes(request, bytes_read);
	close(sfd);
	send(fd, request, bytes_read, 0);
	close(fd);
}


int complete_request_received(char *request) {
	return strstr(request, "\r\n\r\n") == NULL ? 0 : 1;
}


int parse_request(char *request, char *method,
		char *hostname, char *port, char *path) 
{
	if(!complete_request_received(request))
	{
		return 0;
	}

	char *method_begin = request;
	char *method_end = strstr(method_begin, " ");
	int num_copy_bytes = method_end - method_begin;
	strncpy(method, method_begin, num_copy_bytes);
	method[num_copy_bytes] = '\0';

	char temp[128];
	char *temp_begin = strstr(method_end, "/") + 2;
	char *temp_end = strstr(temp_begin, " ");
	num_copy_bytes = temp_end - temp_begin;
	strncpy(temp, temp_begin, num_copy_bytes);
	temp[num_copy_bytes] = '\0';

	char *path_begin = strstr(temp_begin, "/");
	char *path_end = strstr(path_begin, " ");
	num_copy_bytes = path_end - path_begin;
	strncpy(path, path_begin, num_copy_bytes);
	path[num_copy_bytes] = '\0';

	char *hostname_begin = temp;
	char *hostname_end;
	if(strstr(hostname_begin, ":") != NULL)
		hostname_end = strstr(hostname_begin, ":");
	else
		hostname_end = strstr(hostname_begin, "/");
	num_copy_bytes = hostname_end - hostname_begin;
	strncpy(hostname, hostname_begin, num_copy_bytes);
	hostname[num_copy_bytes] = '\0';
	
	char *port_begin = strstr(temp, ":");
	if(port_begin == NULL)
	{
		strncpy(port, "80", 3);
	}
	else
	{
		port_begin++;
		char *port_end = strstr(port_begin, "/");
		num_copy_bytes = port_end - port_begin;
		strncpy(port, port_begin, num_copy_bytes);
		port[num_copy_bytes] = '\0';
	}
	return 1;
}


void test_parser() {
	int i;
	char method[16], hostname[64], port[8], path[64];

       	char *reqs[] = {
		"GET http://www.example.com/index.html HTTP/1.0\r\n"
		"Host: www.example.com\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://www.example.com:8080/index.html?foo=1&bar=2 HTTP/1.0\r\n"
		"Host: www.example.com:8080\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://localhost:1234/home.html HTTP/1.0\r\n"
		"Host: localhost:1234\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://www.example.com:8080/index.html HTTP/1.0\r\n",

		NULL
	};
	
	for (i = 0; reqs[i] != NULL; i++) {
		printf("Testing %s\n", reqs[i]);
		if (parse_request(reqs[i], method, hostname, port, path)) {
			printf("METHOD: %s\n", method);
			printf("HOSTNAME: %s\n", hostname);
			printf("PORT: %s\n", port);
			printf("PATH: %s\n", path);
		} else {
			printf("REQUEST INCOMPLETE\n");
		}
	}
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
