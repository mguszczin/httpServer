#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/inotify.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h> 
#include <errno.h>

#include "server.h"
#include "http_request.h"
#include "http_response.h"
#include "read_from_file.h"
#include "wrap_file_to_html.h"
#include "inotifyConfiguration.h"
#include "buffer_read.h"

#define BUFFER_SIZE 256
char *DIR_PATH = "file_read";


char* get_file_path(char *filename) {
    if (filename == NULL) {
        printf("NULL value of file path\n");
        return NULL;
    }

    // Calculate new size length of FILE_DIR + length of *filename + 1 for null terminator.
    int newsize = strlen(filename) + strlen(DIR_PATH) + 1;
    char *result = malloc(newsize);
    if (!result) {
        perror("malloc failed");
        return NULL;
    }

    // Copy FILE_DIR into result and append the filename.
    strcpy(result, DIR_PATH);
    strcat(result, filename);

    return result;
}

void send_http_update(int clientsocket, char **filepath){
    // Initialize response 
    HttpResponse *httpRes = malloc(sizeof(HttpResponse));
    InitializeHttpResponse(httpRes);

    if(*filepath == NULL) {
        perror("filepath is NULL");
        return;
    }

    // get content of file 
    char *filecontent = getfile(*filepath);


    if(filecontent == NULL) {
        // if there is no file specified in the url - we throw 404
        getHttpStatusLine(httpRes, HTTP_OK);
        getHtmlBodyfromFile("404 Not Found",httpRes);
    }else {
        // if there is specified file we send it with necessary headers 
        getHttpStatusLine(httpRes, HTTP_OK);
        getHtmlBodyfromFile(filecontent,httpRes);
    }
    SendHttpResponse(httpRes, clientsocket);
    freeHttpResponse(httpRes);
}

void send_file_update(int clientsocket, char **buffer, char **prevurl, int size) {
    // if we don't have any url we have some error
    if(*prevurl == NULL) {
        perror("No prev url");
        return;
    }
    struct inotify_event *event;
    for (char *ptr = *buffer; ptr < *buffer + size; ptr += sizeof(struct inotify_event) + event->len) {
        event = (struct inotify_event *) ptr;

        if (event->mask & IN_MODIFY) {
            char *filepath = get_file_path("/index");
            printf("Changed_file_Path : %s", filepath);         // check for errors
            if(strcmp(filepath, *prevurl) == 0) {
                send_http_update(clientsocket, &filepath);
            }
        }
    }
}

void get_http_response(int clientsocket, char **buffer, char **prevurl) {
    HttpRequest *httpreq = malloc(sizeof(HttpRequest));
    initialize_request(httpreq);

    if(assign_request(*buffer, httpreq) < 0) {
        perror("assign of values went wrong");
        free(httpreq);
        return;
    }

    // initialize httpResponse
    HttpResponse *httpRes = malloc(sizeof(HttpResponse));
    InitializeHttpResponse(httpRes);

    // asigin prev ulr and construct file path 
    *prevurl = strdup(httpreq->path);
    char *filepath = get_file_path(httpreq->path);

    if(filepath == NULL) {
        perror("filepath is NULL");
        return;
    }

    // get content of file 
    char *filecontent = getfile(filepath);

    if(filecontent == NULL) {
        getHttpStatusLine(httpRes, HTTP_OK);
        getHtmlBodyfromFile("404 Not Found",httpRes);
    }else {
        getHttpStatusLine(httpRes, HTTP_OK);
        getHtmlBodyfromFile(filecontent,httpRes);
    }
    SendHttpResponse(httpRes, clientsocket);
    freeHttpRequest(httpreq);
    freeHttpResponse(httpRes);
}

void handle_socket(int clientsocket) {
    char *http_buffer = NULL;      // buffer for HTTP request data
    char *inotify_buffer = NULL;   // buffer for inotify events
    char *prevurl = NULL;          // track previous URL 

    int wd, inotify_fd;
    if(!inotifyInitialize(&wd, &inotify_fd, DIR_PATH)){
        perror("handle_socket() - inotify initialized without sucess");
        close(clientsocket);
        return;
    }
    
    // Set both clientsocket and inotify_fd to non-blocking mode
    if (fcntl(clientsocket, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl clientsocket");
    }
    if (fcntl(inotify_fd, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl inotify_fd");
    }

    // Set up poll descriptors
    struct pollfd fds[2];
    fds[0].fd = clientsocket;
    fds[0].events = POLLIN;
    fds[1].fd = inotify_fd;
    fds[1].events = POLLIN;
    while (1) {
        int poll_result = poll(fds, 2, -1);
        if (poll_result < 0) {
            perror("poll");
            break;
        }

        // Handle HTTP request events
        if (fds[0].revents & POLLIN) {
            int status = dynamic_read(clientsocket, &http_buffer);
            if (status > 0) {
                // Process the HTTP request
                get_http_response(clientsocket, &http_buffer, &prevurl);
                free(http_buffer);
                http_buffer = NULL;
            } else if (status == 0) {
                // Connection closed
                break;
            } else {
                if (errno == ECONNRESET) {
                    printf("Connection reset by peer.\n");
                    break;
                }
                perror("dynamic_read (HTTP)");
            }
        }

        // Handle inotify events (file changes)
        if (fds[1].revents & POLLIN) {
            printf("some file changed");
            int status = dynamic_read(inotify_fd, &inotify_buffer);
            if (status > 0) {
                send_file_update(clientsocket, &inotify_buffer, &prevurl, status);
                free(inotify_buffer);
                inotify_buffer = NULL;
            } else if (status < 0) {
                perror("dynamic_read (inotify)");
            }
        }
    }
    printf("BYE");
    free(http_buffer);
    free(inotify_buffer);
    free(prevurl);
    close(clientsocket);
    close(inotify_fd);
}