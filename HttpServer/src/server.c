#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "server.h"
#include "http_request.h"
#include "http_response.h"

#define BUFFER_SIZE 256

void construct_http_response(HttpRequest *req, HttpResponse *res, int clientsocket) {

    if(strcmp(req->path, "/dupa") == 0) {
        res->starting_line.status_code = 200;
        res->starting_line.status_message = strdup("OK");
    }else {
        res->starting_line.status_code = 404;
        res->starting_line.status_message = strdup("Not Found");
    }

    SendHttpResponse(res, clientsocket);
}


void handle_socket(int clientsocket) {

    // read http request from clientsocket
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(clientsocket, buffer, sizeof(buffer) - 1);

    if (bytes_read < 0) {
        perror("read failed");
        close(clientsocket);
        return;
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
    close(clientsocket);
    freeHttpRequest(httpReq);
    freeHttpResponse(httpRes);
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
