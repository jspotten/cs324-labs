#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>


int main(int argc, char *argv[]) 
{
    int content_length = 0;
    if(getenv("CONTENT_LENGTH") != NULL)
        content_length = atoi(getenv("CONTENT_LENGTH"));
    char* query_string = getenv("QUERY_STRING");
    char buffer[content_length+1];
    unsigned int nread = read(0, buffer, content_length);
    buffer[content_length] = '\0';
    char resp_body[2000];
    sprintf(resp_body, "Hello CS324\nQuery string: %s\nRequest body: %s\n", query_string, buffer);
    printf("Content-Type: text/plain\r\nContent-Length: %ld\r\n\r\n", strlen(resp_body));
    printf("%s", resp_body);
}