#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

typedef struct
{
    char *method;
    char *path;
    char *protocol;

    char **headers;
    int header_cnt;

    char *body;

} HttpRequest;

// make sure all the initial values of the data structure are set to null 
void initialize_request(HttpRequest *httpRequest);

// assign values from the httpRequest to the HttpRequest data structure 
// if the format of the first line is invalid or there is a mistake
// function will return -1
int assign_request(char *rawRequest, HttpRequest *httpRequest);

// free the memory allocated by the data stucture
void freeHttpRequest(HttpRequest *req);

#endif