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
int dynamic_read_from_socket(char** http_buffer, int buffer_size, int clientsocket)
{
        int bytes_read = 0;
        int cur = 0;

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

// REMEMBER TO FREE HTTP REQUEST
void choose_response(http_request_t* http_request, int client_socket) 
{       
        if (strcmp(http_request->method, "GET") == 0) {

        }
        http_response_t* http_response = initialize_http_response(HTTP_NOT_FOUND);
        const char* not_found_body =
                "<html>\n"
                "  <head>\n"
                "    <title>404 Not Found</title>\n"
                "    <style>\n"
                "      body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }\n"
                "      h1 { font-size: 50px; color: #cc0000; }\n"
                "      p { font-size: 20px; color: #333333; }\n"
                "    </style>\n"
                "  </head>\n"
                "  <body>\n"
                "    <h1>404</h1>\n"
                "    <p>Page Not Found</p>\n"
                "  </body>\n"
                "</html>\n";
        printf("%ld : strlen\n", strlen(not_found_body));
        //add_body(http_response, not_found_body, MIME_HTML);
        send_http_response(http_response, client_socket);       
        free_http_response(http_response);
}

void handle_socket(int clientsocket)
{
        int buffer_size = 1;
        char* http_buffer = (char*)malloc(sizeof(char*) * buffer_size);
        
        if (!http_buffer) {
                fprintf(stderr, "Failed to allocate memory for http buffer");
                return;
        }
	
        buffer_size = dynamic_read_from_socket(&http_buffer, buffer_size, clientsocket);
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