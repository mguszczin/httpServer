#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/inotify.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h> 

#include "server.h"
#include "http_request.h"
#include "http_response.h"
#include "read_from_file.h"
#include "wrap_file_to_html.h"

#define BUFFER_SIZE 256

void construct_http_response(HttpRequest *req, HttpResponse *res, int clientsocket) {
    ssize_t bytes_read = read(clientsocket, buffer, sizeof(buffer) - 1);

        if (bytes_read < 0) {
            perror("read failed");
            break;
            return;
        }
        if (bytes_read == 0) {
            perror("connection closed");
            break;
        }

        // initialize new httprequest - remember to free 
        HttpRequest *httpReq = malloc(sizeof(HttpRequest));
        initialize_request(httpReq);
        if(assign_request(buffer, httpReq) < 0) {
            perror("assign of values went wrong");
            free(httpReq);
            return;
        }

        // initialize httpResponse
        HttpResponse *httpRes = malloc(sizeof(HttpResponse));
        InitializeHttpResponse(httpRes);

        // construct and send http response
        construct_http_response(httpReq, httpRes, clientsocket);

        // close the socket and free all the memory
        freeHttpRequest(httpReq);
        freeHttpResponse(httpRes);

    if(strcmp(req->path, "/index") == 0) {
        getHttpStatusLine(res, HTTP_OK);
        char *filepath = strdup("file_read/index");
        char *filecontent = getfile(filepath);
        getHtmlBodyfromFile(filecontent,res);

    }else {
        getHttpStatusLine(res, HTTP_NOT_FOUND);
        getHtmlBodyfromFile("404 Not Found",res);
    }

    SendHttpResponse(res, clientsocket);
}

void dynamic_read(int filedescriptor, char **buffer) {
    *buffer = NULL;
    if(filedescriptor < 0) {
        perror("Invalid file descriptor");
        *buffer = NULL;
        return;
    }

    int move = 0;
    int size = 2;  // Start with small buffer but room for '\0'
    *buffer = malloc(size);
    if (!*buffer) {
        perror("malloc failed");
        return;
    }

    ssize_t bytes_read;
    while ((bytes_read = read(filedescriptor, (*buffer) + move, 1)) > 0) {
        move++;

        // Ensure space for next character + null terminator
        if (move >= (size - 1)) {
            size *= 2;
            char *buffer = realloc(*buffer, size);
            if (!buffer) {
                perror("realloc failed");
                free(*buffer);
                *buffer = NULL;
                return;
            }
        }
    }

    // Ensure null termination
    (*buffer)[move] = '\0';
}


void handle_socket(int clientsocket) {

    // buffer to read http request from clientsocket
    char *buffer = NULL;
    char *prevurl = NULL;          // track prev url - know which file was send 
    char *FILE_PATH = "/exec";

    // initialize inotify file descriptor
    int inotify_fd = inotify_init();
    int watch = inotify_add_watch(FILE_PATH, IN_MODIFY);
    if (watch < 0) {
        perror("inotify_add_watch");
        close(inotify_fd);
        close(clientsocket);
        return;
    }
    

    // set socket to a non blocking mode
    // want to avoid potential issues - poll should do just fine 
    fcntl(clientsocket, F_SETFL, O_NONBLOCK);
    fcntl(inotify_fd, F_SETFL, O_NONBLOCK);

    if (fcntl(clientsocket, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl clientsocket");
    }
    if (fcntl(inotify_fd, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl inotify_fd");
    }


    struct pollfd fds[2];
    fds[0].fd = clientsocket;
    fds[0].events = POLLIN;
    fds[1].fd = inotify_fd;
    fds[1].events = POLLIN;

    while(poll(fds,2, -1) >= 0) {
        
        // listen for http request
        // we use and because.events i a bitwise flag 
        if(fds[0].revents & POLLIN) {
            
        }

        // listen for file current file changes 
        if(fds[1].revents & POLLIN) {

        }

    }
    close(clientsocket);
    close(inotify_fd);
}

int start_server(int PORT) {
    int serversocket;

    struct sockaddr_in socketadress;
    socklen_t adresssize = sizeof(socketadress);
    
    if((serversocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {         // creating socket for tcp connections
        perror("Socket failed");
        exit(EXIT_FAILURE);
        return -1;
    }

    socketadress.sin_addr.s_addr = INADDR_ANY;    
    socketadress.sin_family = AF_INET;
    socketadress.sin_port = htons(PORT); 

    if(bind(serversocket, (struct sockaddr *)&socketadress, adresssize) < 0) {       // bind adress to a socket 
        perror("Binding went wrong");
        close(serversocket);
        exit(EXIT_FAILURE);
        return -1;
    }

    int max_queue = 5;
    if(listen(serversocket, max_queue) < 0) {       // make socket listen to calls 
        perror("Listening failed");
        close(serversocket);
        exit(EXIT_FAILURE);
        return -1;
    }

    // make new socket for http response
    int clientsocket;                             
    while((clientsocket = accept(serversocket, (struct sockaddr *)&socketadress, &adresssize)) >= 0) {
        // create concurrent connection
        pid_t procesid = fork();
        
        if(procesid == 0) {
            close(serversocket);
            handle_socket(clientsocket);
            exit(0);
        }else if(procesid > 0) {
            close(clientsocket);
            continue;
        }else {
            perror("Forking went wrong");
            close(serversocket);
            return -1;
            break;
        }
    }

    close(serversocket);

    return 0;
}
