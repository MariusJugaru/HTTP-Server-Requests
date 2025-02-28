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
#include "./parson/parson.h"
#include "client_functions.h"

int is_command(char *command1, char *command2) {
    if (strcmp(command1, command2) == 0) {
        return 1;
    }
    return 0;
}

char *extract_value(char *response, char *header) {
    char *aux = strstr(response, header);
    if (aux == NULL) {
        return NULL;
    }

    char *end_of_line = strstr(aux, "\r\n");

    int val_len = (int)(end_of_line - aux) - strlen(header) - 2;

    char *val = malloc(val_len);
    strncpy(val, aux  + strlen(header) + 2, val_len);

    return val;
}

char *extract_payload(char *response) {
    char *aux = strstr(response, "\r\n\r\n");
    char *value = extract_value(response, "Content-Length");
    char *payload = NULL;
    
    value = extract_value(response, "Content-Length");

    if (value != NULL) {
        int content_len;
        sscanf(value, "%d", &content_len);

        payload = malloc(content_len + 1);
        strncpy(payload, aux + 4, content_len);
        payload[content_len] = '\0';
    }

    free(value);
    return payload;
}

void cookies_to_array(char *response, char ***cookies_session, int *cookies_count) {
    char *cookies = extract_value(response, "Set-Cookie");
    if (cookies == NULL) {
        exit(EXIT_FAILURE);
    }
    char *cookie = strtok(cookies, ";");

    if (*cookies_count == 0) {
        *cookies_session = (char **)malloc(10 * sizeof(char *));
        if (*cookies_session == NULL) {
            exit(EXIT_FAILURE);
        }
    }

    while (cookie != NULL) {
        while (*cookie == ' ') {
            cookie++;
        }

        (*cookies_session)[*cookies_count] = strdup(cookie);
        if ((*cookies_session)[*cookies_count] == NULL) {
            exit(EXIT_FAILURE);
        }

        (*cookies_count)++;

        if (*cookies_count % 10 == 0) {
            *cookies_session = (char **)realloc(*cookies_session, (*cookies_count + 10) * sizeof(char *));
            if (*cookies_session == NULL) {
                exit(EXIT_FAILURE);
            }
        }

        cookie = strtok(NULL, ";");
    }

    free(cookies);
}

void run_client(int sockfd) {
    // json
    JSON_Value *root_value;
    JSON_Object *root_object;
    char *serialized_string = NULL;

    char *response_payload;
    char command[40];
    char *message;
    char *response;
    char **cookies_session;
    char *token = NULL;

    int cookies_count = 0;

    while(1) {
        fgets(command, sizeof(command), stdin);
        command[strlen(command) - 1] = '\0';

        if (is_command(command, "register")) {
            char username[100], password[100];

            printf("username=");
            fgets(username, sizeof(username), stdin);
            username[strlen(username) - 1] = '\0';

            printf("password=");
            fgets(password, sizeof(password), stdin);
            password[strlen(password) - 1] = '\0';
            if (strchr(username, ' ') != NULL) {
                printf("Error: Invalid username!\n");
                continue;
            }
            
            // creating the json data
            root_value = json_value_init_object();
            root_object = json_value_get_object(root_value);
            json_object_set_string(root_object, "username", username);
            json_object_set_string(root_object, "password", password);
            serialized_string = json_serialize_to_string_pretty(root_value);

            // creating and sending the request
            message = compute_post_request(SERVER_IP, "/api/v1/tema/auth/register", "application/json", serialized_string, NULL, 0, NULL);
            send_to_server(sockfd, message);
            
            // receiving the response
            response = receive_from_server(sockfd);
            response_payload = extract_payload(response);

            if (strcmp(response_payload, "ok") == 0) {
                printf("User registration was a success!\n");
            } else {
                printf("Error: The username %s is already taken\n", username);
            }

            // freeing the alocated memory
            free(response_payload);
            free(message);
            free(response);
            json_free_serialized_string(serialized_string);
            json_value_free(root_value);

        } else if (is_command(command, "login")) {
            char username[100], password[100];

            printf("username=");
            fgets(username, sizeof(username), stdin);
            username[strlen(username) - 1] = '\0';

            printf("password=");
            fgets(password, sizeof(password), stdin);
            password[strlen(password) - 1] = '\0';
            
            // creating the json data
            root_value = json_value_init_object();
            root_object = json_value_get_object(root_value);
            json_object_set_string(root_object, "username", username);
            json_object_set_string(root_object, "password", password);
            serialized_string = json_serialize_to_string_pretty(root_value);

            // creating and sending the request
            message = compute_post_request(SERVER_IP, "/api/v1/tema/auth/login", "application/json", serialized_string, NULL, 0, NULL);
            send_to_server(sockfd, message);
            
            // receiving the response
            response = receive_from_server(sockfd);
            response_payload = extract_payload(response);

            if (strcmp(response_payload, "ok") == 0) {
                // printf("%s\n", response);
                printf("User login was a success\n");
                cookies_to_array(response, &cookies_session, &cookies_count);
            } else {
                printf("Error: Credentials are not good!\n");
            }

            // freeing the alocated memory
            free(response_payload);
            free(message);
            free(response);
            json_free_serialized_string(serialized_string);
            json_value_free(root_value);
        } else if (is_command(command, "enter_library")) {
            message = compute_get_request(SERVER_IP, "/api/v1/tema/library/access", NULL, cookies_session, cookies_count, NULL);
            send_to_server(sockfd, message);

            response = receive_from_server(sockfd);
            response_payload = extract_payload(response);

            if (strstr(response_payload, "\"error\"") != 0) {
                printf("Error: Credentials are not good!\n");
            } else {
                // Parsing the JSON from the payload
                JSON_Value *root_value = json_parse_string(response_payload);
                JSON_Object *root_object = json_value_get_object(root_value);

                const char *val = json_object_get_string(root_object, "token");
                token = strdup(val);
                printf("The access to the library was a success!\n");
                json_value_free(root_value);
            }

            // freeing the alocated memory
            free(message);
            free(response);
            free(response_payload);
        } else if (is_command(command, "get_books")) {
            message = compute_get_request(SERVER_IP, "/api/v1/tema/library/books", NULL, cookies_session, cookies_count, token);
            send_to_server(sockfd, message);

            response = receive_from_server(sockfd);
            response_payload = extract_payload(response);

            if (strstr(response_payload, "\"error\"") == NULL) {
                printf("%s\n", response_payload);
            } else {
                printf("Error!\n");
            }
            
            // freeing the alocated memory
            free(message);
            free(response);
            free(response_payload);
        } else if (is_command(command, "get_book")) {
            char id[20];

            printf("id=");
            fgets(id, sizeof(id), stdin);
            id[strlen(id) - 1] = '\0';

            char books_path[50];
            strcpy(books_path, "/api/v1/tema/library/books/");
            strcat(books_path, id);

            message = compute_get_request(SERVER_IP, books_path, NULL, cookies_session, cookies_count, token);
            send_to_server(sockfd, message);

            response = receive_from_server(sockfd);
            response_payload = extract_payload(response);

            printf("%s\n", response_payload);

            // freeing the alocated memory
            free(message);
            free(response);
            free(response_payload);            
        } else if (is_command(command, "add_book")) {
            char title[100], author[40], genre[40], publisher[40], page_count_char[10];
            int page_count;
            int ok = 1;


            // reading the data
            printf("title=");
            fgets(title, sizeof(title), stdin);
            title[strlen(title) - 1] = '\0';
            if (strlen(title) == 0) {
                ok = 0;
            }

            printf("author=");
            fgets(author, sizeof(author), stdin);
            author[strlen(author) - 1] = '\0';
            if (strlen(author) == 0) {
                ok = 0;
            }

            printf("genre=");
            fgets(genre, sizeof(genre), stdin);
            genre[strlen(genre) - 1] = '\0';
            if (strlen(genre) == 0) {
                ok = 0;
            }

            printf("publisher=");
            fgets(publisher, sizeof(publisher), stdin);
            publisher[strlen(publisher) - 1] = '\0';
            if (strlen(publisher) == 0) {
                ok = 0;
            }

            printf("page_count=");
            fgets(page_count_char, sizeof(page_count_char), stdin);
            page_count_char[strlen(page_count_char) - 1] = '\0';
            if (strlen(page_count_char) == 0) {
                ok = 0;
            } else {
                sscanf(page_count_char, "%d", &page_count);
                if (page_count == 0 && *page_count_char - '0' != 0) {
                    ok = 0;
                }
            }

            if (ok == 0) {
                printf("Error!\n");
                continue;
            }

            // creating the json data
            root_value = json_value_init_object();
            root_object = json_value_get_object(root_value);
            json_object_set_string(root_object, "title", title);
            json_object_set_string(root_object, "author", author);
            json_object_set_string(root_object, "genre", genre);
            json_object_set_string(root_object, "publisher", publisher);
            json_object_set_number(root_object, "page_count", page_count);
            serialized_string = json_serialize_to_string_pretty(root_value);

            // creating and sending the request
            message = compute_post_request(SERVER_IP, "/api/v1/tema/library/books", "application/json", serialized_string, cookies_session, cookies_count, token);
            send_to_server(sockfd, message);
            
            // receiving the response
            response = receive_from_server(sockfd);
            response_payload = extract_payload(response);

            if (strstr(response_payload, "\"error\"") == NULL) {
                printf("The book was added with success\n");
            } else {
                printf("Error!\n");
            }

            // freeing the alocated memory
            free(message);
            free(response);
            free(response_payload);
            json_free_serialized_string(serialized_string);
            json_value_free(root_value);
        } else if (is_command(command, "delete_book")) {
            char id[20];

            printf("id=");
            fgets(id, sizeof(id), stdin);
            id[strlen(id) - 1] = '\0';

            char books_path[50];
            strcpy(books_path, "/api/v1/tema/library/books/");
            strcat(books_path, id);

            message = compute_delete_request(SERVER_IP, books_path, cookies_session, cookies_count, token);
            send_to_server(sockfd, message);

            response = receive_from_server(sockfd);
            response_payload = extract_payload(response);

            if (strstr(response_payload, "\"error\"") == NULL) {
                printf("The book was deleted with success\n");
            } else {
                printf("Error!\n");
            }

            // freeing the alocated memory
            free(message);
            free(response);
            free(response_payload); 
        } else if (is_command(command, "logout")) {
            message = compute_get_request(SERVER_IP, "/api/v1/tema/auth/logout", NULL, cookies_session, cookies_count, token);
            send_to_server(sockfd, message);

            response = receive_from_server(sockfd);
            response_payload = extract_payload(response);

            if (strstr(response_payload, "\"error\"") != 0) {
                printf("Error: Credentials are not good!\n");
            } else {
                printf("Logout was a success!\n");
            }

            // freeing the alocated memory
            for(int i = 0; i < cookies_count; i++) {
                free(cookies_session[i]);
            }
            free(cookies_session);
            cookies_count = 0;

            free(message);
            free(response);
            free(response_payload);
        } else if (is_command(command, "exit")) {
            free(token);
            break;
        }
    }
}