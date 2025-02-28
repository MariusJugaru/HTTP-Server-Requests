#ifndef _CLIENT_FUNCTIONS_
#define _CLIENT_FUNCTIONS_

#define PORT 8080
#define SERVER_IP "34.246.184.49"

// check if two commands are equal
int is_command(char *command1, char *command2);

// runs the clients on a loop and waits for commands 
void run_client(int sockfd);

// extracts the value from a header in a http response
char *extract_value(char *response, char *header);

// extracts the payload from a response
char *extract_payload(char *response);

// extracts the cookies from a response and adds them to an array
void cookies_to_array(char *response, char ***cookies_session, int *cookies_count);

#endif