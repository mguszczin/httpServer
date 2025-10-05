#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <unistd.h>

#include "server/http_request.h"
#include "server/http_response.h"
#include "inotify/inotifyConfiguration.h"
#include "file_reader/read_from_file.h"
#include "server/server.h"

#define BUFFER_SIZE 256
char *DIR_PATH = "file_read";

/*
* Dynamic read from socket using realloc so that space allocation for buffer is almost constant. 
* Http buffer ends with null terminator so that the end of string is marked (made for functions from string.h lib)
*/
int dynamic_read_from_socket(char **http_buffer, int buffer_size, int clientsocket)
{
        int bytes_read = 0;
        int cur = 0;

        if (*http_buffer == NULL) {
                *http_buffer = malloc(buffer_size);
                if (!*http_buffer) return -1;
        }

        while ((bytes_read = recv(clientsocket, *http_buffer + cur, buffer_size - cur - 1, 0)) > 0) {
                cur += bytes_read;
                (*http_buffer)[cur] = '\0'; 
      
                if (strstr(*http_buffer, "\r\n\r\n")) {
                        break;
                }
        
                if (cur >= buffer_size - 1) {
                        buffer_size *= 2;
                        char *tmp = realloc(*http_buffer, buffer_size);
                        if (!tmp) {
                                fprintf(stderr, "Failed to reallocate while dynamically reading\n");
                                free(*http_buffer);
                                return -1;
                        }
                        *http_buffer = tmp;
                }
        }

        if (bytes_read < 0) 
                return -1;

        (*http_buffer)[cur] = '\0';
        return cur;
}

void choose_response(http_request_t* http_request, int client_socket) 
{       
        if (strcmp(http_request->method, "GET") == 0) {
                handle_get_request(http_request, client_socket);
                return;
        }
        /* No other request supported yet */
        send_http_error_response(HTTP_BAD_REQUEST, client_socket);
        return;
}

void handle_socket(int clientsocket)
{
        int buffer_size = 2;
        char* http_buffer = (char*)malloc(sizeof(char*) * buffer_size);
        
        if (!http_buffer) {
                fprintf(stderr, "Failed to allocate memory for http buffer");
                return;
        }

	
        buffer_size = dynamic_read_from_socket(&http_buffer, buffer_size, clientsocket);
        printf("Received request:\n%s\n", http_buffer);
        printf("Buffer size: %d\n", buffer_size);
        if (buffer_size == -1) {
                fprintf(stderr, "Something went wrong when reading from socket");
                free(http_buffer);
                return;
        }
        
        http_request_t* http_request = assign_request(http_buffer);
        if (!http_request) {
                fprintf(stderr, "Failed to allocate memory for http request");
                free(http_buffer);
                return;
        }
        choose_response(http_request, clientsocket);
        printf("connection closed \n");
        free(http_request);
	free(http_buffer);
}