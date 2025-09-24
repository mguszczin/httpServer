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

#include "buffer_read.h"
#include "http_request.h"
#include "http_response.h"
#include "inotifyConfiguration.h"
#include "read_from_file.h"
#include "server.h"
#include "wrap_file_to_html.h"

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

char *push_front(char *front, char *back)
{
	// Calculate new size +1 for null terminator
	int newsize = strlen(front) + strlen(back) + 1;
	char *newString = malloc(sizeof(char) * newsize);
	if (!newString) {
		perror("malloc failed");
		return NULL;
	}

	// copy front and back to newString
	strcpy(newString, front);
	strcat(newString, back);

	return newString;
}

void send_http_update(int clientsocket, char **filepath)
{
	// Initialize response
	HttpResponse *httpRes = malloc(sizeof(HttpResponse));
	InitializeHttpResponse(httpRes);

	if (*filepath == NULL) {
		perror("filepath is NULL");
		return;
	}

	// get content of file
	char *filecontent = getfile(*filepath);
	addHeader(httpRes, "Connection: keep-alive");
	if (filecontent == NULL) {
		// if there is no file specified in the url - we throw 404
		getHttpStatusLine(httpRes, HTTP_NOT_FOUND);
		getHtmlBodyfromFile("404 Not Found", httpRes);
	} else {
		// if there is specified file we send it with necessary headers
		getHttpStatusLine(httpRes, HTTP_OK);
		getHtmlBodyfromFile(filecontent, httpRes);
	}
	SendHttpResponse(httpRes, clientsocket);
	freeHttpResponse(httpRes);
	free(filecontent);
}

void send_file_update(int clientsocket, char **buffer, char **prevurl, int size)
{
	// if we don't have any url we have some error
	if (*prevurl == NULL) {
		perror("No prev url");
		return;
	}
	struct inotify_event *event;
	for (char *ptr = *buffer; ptr < *buffer + size;
	     ptr += sizeof(struct inotify_event) + event->len) {
		event = (struct inotify_event *)ptr;

		if ((event->mask & IN_MODIFY) && !(event->mask & IN_ISDIR)) {
			char *updPath = push_front("/", event->name);
			printf("Changed_file_Path : %s\n",
			       updPath); // check for errors
			printf("Prev_Url : %s\n", *prevurl);
			if (strcmp(updPath, *prevurl) == 0) {
				char *filepath = get_file_path(updPath);
				printf("FIle path : %s\n", filepath);
				send_http_update(clientsocket, &filepath);
				free(filepath);
			}
			free(updPath);
		}
	}
}

void get_http_response(int clientsocket, char **buffer, char **prevurl)
{
	HttpRequest *httpreq = malloc(sizeof(HttpRequest));
	initialize_request(httpreq);

	if (assign_request(*buffer, httpreq) < 0) {
		perror("assign of values went wrong");
		free(httpreq);
		return;
	}

	// asigin prev ulr and construct file path
	char *filepath = get_file_path(httpreq->path);

	if (strcmp(httpreq->path, "/favicon.ico") != 0)
		*prevurl = strdup(httpreq->path);

	if (filepath == NULL) {
		perror("filepath is NULL");
		return;
	}

	send_http_update(clientsocket, &filepath);

	freeHttpRequest(httpreq);
}

int dynamic_read_from_socket(char** http_buffer, size_t buffer_size, int clientsocket)
{
        ssize_t bytes_read = 0;
        size_t cur = 0;

        while ((bytes_read = recv(clientsocket, *http_buffer + cur,buffer_size - cur, 0)) > 0) {
                cur += bytes_read;

                if ((buffer_size) <= cur) {
                        buffer_size *= 2;
                        char *tmp = realloc(*http_buffer, buffer_size);
                        if(!tmp) {
                                fprintf(stderr, "Failed to reallocate while dynamically reading");
                                free(*http_buffer);
                                return -1;
                        }
                        *http_buffer = tmp;
                }
        }

        if (bytes_read < 0) 
                return -1;
        
        (*http_buffer)[cur] = '\0';
        return buffer_size;
}

void handle_socket(int clientsocket)
{
        size_t buffer_size = 1;
        char* http_buffer = (char*)malloc(sizeof(char*) * buffer_size);
        
        if (!http_buffer){
                fprintf(stderr, "Failed to allocate memory for http buffer");
                return;
        }
	
        buffer_size = dynamic_read_from_socket(&http_buffer, &buffer_size, clientsocket);
        if (buffer_size == -1) {
                fprintf(stderr, "Something went wrong when reading from socket");
                return;
        }

        HttpRequest http_request;
        initialize_request(&http_request);
        assign_request(http_buffer, &http_request);

        

	free(http_buffer);
}