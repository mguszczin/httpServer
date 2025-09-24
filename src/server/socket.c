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

#include "http_request.h"
#include "http_response.h"
#include "inotifyConfiguration.h"
#include "read_from_file.h"
#include "server.h"

#define BUFFER_SIZE 256
char *DIR_PATH = "file_read";

char *get_file_path(char *filename)
{
	if (filename == NULL) {
		printf("NULL value of file path\n");
		return NULL;
	}

	// Calculate new size length of FILE_DIR + length of *filename + 1 for
	// null terminator.
	int newsize = strlen(filename) + strlen(DIR_PATH) + 1;
	char *result = malloc(newsize);
	if (!result) {
		perror("malloc failed");
		return NULL;
	}

	// Copy FILE_DIR into result and append the filename.
	// strcpy first because we have unitialized memory
	strcpy(result, DIR_PATH);
	strcat(result, filename);

	return result;
}

int dynamic_read_from_socket(char** http_buffer, size_t buffer_size, int clientsocket)
{
        ssize_t bytes_read = 0;
        size_t cur = 0;

        while ((bytes_read = recv(clientsocket, *http_buffer + cur,buffer_size - cur, 0)) > 0) {
                cur += bytes_read;

                if ((buffer_size) > cur) 
                        continue;

                buffer_size *= 2;
                char *tmp = realloc(*http_buffer, buffer_size);
                if(!tmp) {
                        fprintf(stderr, "Failed to reallocate while dynamically reading");
                        free(*http_buffer);
                        return -1;
                }
                *http_buffer = tmp;
                
        }

        if (bytes_read < 0) 
                return -1;
        
        (*http_buffer)[cur] = '\0';
        return buffer_size;
}

void send_response(http_request_t* http_request, int clientsocket) 
{
        if (strcmp(http_request->method, "GET") == 0) {

        } else {

        }

        http_response_t* http_response;

        if (!http_response) {
                fprintf(stderr, "Something went wrong when creating http response");
                return;
        }

        int status = send(clientsocket, http_response, strlen(http_response), 0);
        if (status == -1) 
                fprintf(stderr, "Sending data to socket went wrong");
}

void handle_socket(int clientsocket)
{
        size_t buffer_size = 1;
        char* http_buffer = (char*)malloc(sizeof(char*) * buffer_size);
        
        if (!http_buffer) {
                fprintf(stderr, "Failed to allocate memory for http buffer");
                return;
        }
	
        buffer_size = dynamic_read_from_socket(&http_buffer, &buffer_size, clientsocket);
        if (buffer_size == -1) {
                fprintf(stderr, "Something went wrong when reading from socket");
                free(http_buffer);
                return;
        }

        http_request_t* http_request = (http_buffer, &http_request);
        if (!http_request) {
                fprintf(stderr, "Failed to allocate memory for http request");
                free(http_buffer);
                return;
        }

        send_response(http_request, clientsocket);

	free(http_buffer);
}