#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

typedef enum {
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_BAD_REQUEST = 400,
    HTTP_NOT_FOUND = 404,
    HTTP_INTERNAL_ERROR = 500
} HttpStatusCode;

typedef enum {
    HTML,
    TEXT
} ContentType;

typedef struct{
    int status_code;
    char *status_message;
} StartingLine;

typedef struct {
    StartingLine starting_line;
    
    char **headers;  
    int header_count;
    
    char *body;
    int body_size;
} HttpResponse;


void InitializeHttpResponse(HttpResponse *res);

int SendHttpResponse(HttpResponse *res, int clientSocket);

void freeHttpResponse(HttpResponse *res);

void getHttpStatusLine(HttpResponse *res, HttpStatusCode code);

void addHeader(HttpResponse *res, char *header);

void addBody(HttpResponse *res, char *body, ContentType type);

#endif