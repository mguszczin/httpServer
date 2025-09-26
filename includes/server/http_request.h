#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

typedef struct {
	char *method;
	char *path;
	char *protocol;

	char **headers;
	int header_cnt;

	char *body;

} http_request_t;

http_request_t* assign_request(char *raw_request);

void freeHttpRequest(http_request_t *req);

#endif 