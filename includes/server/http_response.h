#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

/*
Status code our functions will be using as allowed input
*/
typedef enum {
        HTTP_OK = 200,
	HTTP_CREATED = 201,
	HTTP_BAD_REQUEST = 400,
	HTTP_NOT_FOUND = 404,
	HTTP_INTERNAL_ERROR = 500
} HttpStatusCode;

typedef enum { HTML, TEXT } ContentType;

typedef struct {
	int status_code;
	char *status_message;
} StartingLine;

typedef struct {
	StartingLine starting_line;

	char **headers;
	int header_count;

	char *body;
	int body_size;
} http_response_t;

void InitializeHttpResponse(http_response_t *res);

int SendHttpResponse(http_response_t *res, int clientSocket);

void freeHttpResponse(http_response_t *res);

void getHttpStatusLine(http_response_t *res, HttpStatusCode code);

void addHeader(http_response_t *res, char *header);

void addBody(http_response_t *res, char *body, ContentType type);

#endif