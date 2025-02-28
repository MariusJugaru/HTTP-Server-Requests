#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "client_functions.h"

int main(int argc, char *argv[])
{
    int sockfd = open_connection(SERVER_IP, PORT, AF_INET, SOCK_STREAM, 0);

    run_client(sockfd);

    close_connection(sockfd);

    return 0;
}
