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
    // strcpy first because we have unitialized memory 
    strcpy(result, DIR_PATH);
    strcat(result, filename);

    return result;
}

char *push_front(char *front, char *back) {
    // Calculate new size +1 for null terminator
    int newsize = strlen(front) + strlen(back) + 1;
    char *newString = malloc(sizeof(char) * newsize);
    if(!newString) {
        perror("malloc failed");
        return NULL;
    }

    // copy front and back to newString
    strcpy(newString, front);
    strcat(newString, back);

    return newString;
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
    addHeader(httpRes, "Connection: keep-alive");
    if(filecontent == NULL) {
        // if there is no file specified in the url - we throw 404
        getHttpStatusLine(httpRes, HTTP_NOT_FOUND);
        getHtmlBodyfromFile("404 Not Found",httpRes);
    }else {
        // if there is specified file we send it with necessary headers 
        getHttpStatusLine(httpRes, HTTP_OK);
        getHtmlBodyfromFile(filecontent,httpRes);
    }
    SendHttpResponse(httpRes, clientsocket);
    freeHttpResponse(httpRes);
    free(filecontent);
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

        if ((event->mask & IN_MODIFY) && !(event->mask & IN_ISDIR)) {
            char *updPath = push_front("/", event->name);
            printf("Changed_file_Path : %s\n", updPath);         // check for errors
            printf("Prev_Url : %s\n", *prevurl);
            if(strcmp(updPath, *prevurl) == 0) {
                char *filepath = get_file_path(updPath);
                printf("FIle path : %s\n", filepath);
                send_http_update(clientsocket, &filepath);
                free(filepath);
            }
            free(updPath);
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

    // asigin prev ulr and construct file path 
    char *filepath = get_file_path(httpreq->path);

    if (strcmp(httpreq->path, "/favicon.ico") != 0) *prevurl = strdup(httpreq->path);

    if(filepath == NULL) {
        perror("filepath is NULL");
        return;
    }

    send_http_update(clientsocket, &filepath);

    freeHttpRequest(httpreq);
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
            int status = dynamic_read(clientsocket, &http_buffer, SMALL_CHUNK);
            if (status > 0) {
                // Process the HTTP request
                get_http_response(clientsocket, &http_buffer, &prevurl);
                free(http_buffer);
                http_buffer = NULL;
            } else if (status == 0) {
                // Connection closed
                printf("connection closed by socket\n");
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
            int status = static_read(inotify_fd, &inotify_buffer, INOTIFY_RECOMMENDED);
            if (status > 0) {
                send_file_update(clientsocket, &inotify_buffer, &prevurl, status);
                free(inotify_buffer);
                inotify_buffer = NULL;
            } else if (status < 0) {
                perror("dynamic_read (inotify) doesn't work - 191 socket.c");
            }
            printf("send update");
        }
    }
    printf("BYE");
    free(http_buffer);
    free(inotify_buffer);
    free(prevurl);
    close(clientsocket);
    close(inotify_fd);
}