#include "http_request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void initialize_request(HttpRequest *httpRequest)
{
	// ensure all values are null and 0
	httpRequest->method = NULL;
	httpRequest->path = NULL;
	httpRequest->protocol = NULL;
	httpRequest->header_cnt = 0;
	httpRequest->headers = NULL;
	httpRequest->body = NULL;
}

// the struct must have method, path and protocol if not -1
int assign_request(char *rawRequest, HttpRequest *httpRequest)
{

	// copy request so we don't modify the main string
	char *reqcp = strdup(rawRequest);
	char *requestLine = strtok(reqcp, "\r\n");

	// ensure the valid format
	if (!requestLine) {
		free(reqcp);
		return -1;
	}

	// initialize requestLine elements
	char *method = strtok(requestLine, " ");
	char *path = strtok(NULL, " ");
	char *protocol = strtok(NULL, " ");

	// ensure the valid request Line formats
	if (!method || !path || !protocol) {
		free(reqcp);
		return -1;
	}

	httpRequest->method = strdup(method);
	httpRequest->path = strdup(path);
	httpRequest->protocol = strdup(protocol);

	// dynamically allocate all headers
	while ((requestLine = strtok(NULL, "\r\n")) &&
	       strlen(requestLine) > 0) {
		httpRequest->header_cnt++;
		char **tmp = realloc(httpRequest->headers,
				     httpRequest->header_cnt * sizeof(char *));
		if (tmp == NULL) {
			perror("realloc failed");
			httpRequest->header_cnt--; // rollback the count
			break;
		}
		httpRequest->headers = tmp;
		httpRequest->headers[httpRequest->header_cnt - 1] =
		    strdup(requestLine);
	}

	// check for body
	char *body = strtok(NULL, "\r\n");
	if (body) {
		httpRequest->body = strdup(body);
	}

	free(reqcp);
	return 1;
}

void freeHttpRequest(HttpRequest *req)
{
	// free the allocated space
	free(req->method);
	free(req->path);
	free(req->protocol);

	for (int i = 0; i < req->header_cnt; i++) {
		free(req->headers[i]);
	}

	free(req->headers);
	free(req->body);
	free(req);
}