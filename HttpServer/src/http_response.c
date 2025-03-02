#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "http_response.h"

static const char* status_mappings[] = {
    [200] = "OK",
    [201] = "Created",
    [400] = "Bad Request",
    [404] = "Not Found",
    [500] = "Internal Server Error"
};


void InitializeHttpResponse(HttpResponse *res){
    res->starting_line = 
    (StartingLine){.status_code = -1, .status_message = strdup("")};

    res->header_count = 0;
    res->headers = NULL;

    res->body = strdup("");
    res->body_size = 0;
}

void get_StartLine_response(HttpResponse *res, char **startline) {

    // calculate StartingLine size - +1 for null terminator
    int StartLine_size = 
    snprintf(NULL, 0, "HTTP/1.1 %d %s\r\n",
    res->starting_line.status_code, res->starting_line.status_message) + 1;

    *startline = (char *)malloc(StartLine_size * sizeof(char));
    if(*startline == NULL) {
        perror("Malloc of Starting Line went wrong");
        return;
    }

    int status = snprintf(*startline, StartLine_size, "HTTP/1.1 %d %s\r\n", 
    res->starting_line.status_code, res->starting_line.status_message);
    if(status < 0) {
        perror("snprintf on starting line went wrong");
        free(*startline);
        return;
    }
}

void get_Headers_response(HttpResponse *res, char **headers) {
    char *headerLine = strdup("");
    int crlf_len = strlen("\r\n");

    for(int i = 0; i < res->header_count; i++) {
        // Calculate new length
        int current_len = strlen(headerLine);
        int new_part_len = strlen(res->headers[i]) + crlf_len;
        int total_len = current_len + new_part_len + 1; // +1 for null terminator

        // realloc for new data
        char *temp = realloc(headerLine, total_len);
        if (temp == NULL) {
            perror("realloc failed");
            free(headerLine);
            return;
        }
        headerLine = temp;

        // Append the header and CRLF
        strcat(headerLine, res->headers[i]);
        strcat(headerLine, "\r\n");
    }
    
    int current_len = strlen(headerLine);
    char *temp = realloc(headerLine, current_len + crlf_len + 1); // +1 for null
    if (temp == NULL) {
        perror("realloc for final CRLF failed");
        free(headerLine);
        return;
    }
    headerLine = temp;
    strcat(headerLine, "\r\n");

    // Assign result to output parameter
    *headers = headerLine;
}

void get_Body_response(HttpResponse *res, char **body) {

    // calculate body size - +1 for null terminator
    int body_size = 
    snprintf(NULL, 0, "%s", res->body) + 1;

    *body = (char *)malloc(body_size * sizeof(char));
    if(body == NULL) {
        perror("Malloc of Body went wrong");
        return;
    }

    int status = snprintf(*body, body_size,"%s", res->body);
    if(status < 0) {
        perror("Printing went wrong");
        free(body);
        return;
    }
}

// function uses get_StartLine_response, get_Headers_response and get_Body_response
int SendHttpResponse(HttpResponse *res, int clientSocket){

    char *StartLine = NULL;
    char *Headers = NULL;
    char *Body = NULL;

    // check i Response exists
    if(res->starting_line.status_code == -1) return -1;

    // get all the needed responses
    get_StartLine_response(res, &StartLine);
    get_Headers_response(res, &Headers);
    get_Body_response(res, &Body);

    char *response = NULL;
    int response_size =                   
    snprintf(NULL, 0, "%s%s%s", StartLine, Headers, Body) + 1; // +1 for null terminator

    response = (char *)malloc(response_size * sizeof(char));
    if(response == NULL){
        perror("Malloc of HttpResponse went wrong");
        return -1;
    }

    // merge all three parts 
    snprintf(response, response_size, "%s%s%s", StartLine, Headers, Body);

    // send http response to clientsocket
    if(send(clientSocket, response, response_size - 1,0) < 0){
        perror("write failed");
    }
    free(response);
    return 1;
}

void freeHttpResponse(HttpResponse *res){
    free(res->starting_line.status_message);
    
    for(int i = 0; i < res->header_count; i++) {
        free(res->headers[i]);
    }

    free(res->body);
    free(res);
}

// get Http response of Chosen code use status_mappings
void getHttpResponse(HttpResponse *res, HttpStatusCode code) {
    res->starting_line = 
    (StartingLine){.status_code = code, .status_message = status_mappings[code]};
}