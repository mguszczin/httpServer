#include "http_request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char *method;
	char *path;
	char *protocol;

	char **headers;
	int header_cnt;

	char *body;

} http_request_t;

void freeHttpRequest(http_request_t *req)
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
}

int get_request_line(char** request_line, http_request_t** http_request) 
{
	char *method = strtok(*request_line, " ");
	char *path = strtok(NULL, " ");
	char *protocol = strtok(NULL, " ");

	if (!method || !path || !protocol) {
		return -1;
	}

        char* cp_method = strdup(method);
        char* cp_path = strdup(path);
        char* cp_protocol = strdup(protocol); 

        if (!cp_method || !cp_path || !cp_protocol) {
                return -1;
        }

	(*http_request)->method = cp_method;
	(*http_request)->path = cp_path;
	(*http_request)->protocol = cp_protocol;


        return 0;
}


int get_headers(char **request_line, http_request_t** http_request)
{
        // dynamically allocate all headers
	while ((request_line = strtok(NULL, "\r\n")) &&
	       strlen(request_line) > 0) {
        
		(*http_request)->header_cnt++;
		char **tmp = realloc((*http_request)->headers,
				     (*http_request)->header_cnt * sizeof(char *));
		if (tmp == NULL) {
			perror("realloc failed");
			(*http_request)->header_cnt--; // rollback the count
			return -1;
		}
		(*http_request)->headers = tmp;

                char *new_header_line = strdup(request_line);
                if(!new_header_line) {
                        perror("strdup failed in get_headers");
                        return -1;
                }
		(*http_request)->headers[(*http_request)->header_cnt - 1] =
		    new_header_line;
                
	}
        return 0;
}

int get_body(char **request_line, http_request_t** http_request)
{
        char *body = strtok(NULL, "\r\n");
	if (!body) 
                return 0;
        
        char* new_body = strdup(body);
        if (!new_body)
                return -1;
        (*http_request)->body = new_body;

        return 0;
}

// the struct must have method, path and protocol if not -1
http_request_t* assign_request(char *raw_request)
{       
        http_request_t *http_request = calloc(1, sizeof(http_request_t));
	if (!http_request)
		return NULL;

	// copy request so we don't modify the main string
	char *reqcp = strdup(raw_request);
	char *request_line = strtok(reqcp, "\r\n");
	if (!request_line) {
		free(reqcp);
                free(http_request);
		return NULL;
	}
        // need to free reqcp on failure 
	if (get_request_line(&request_line, &http_request) == -1 ||
	get_header(&request_line, &http_request) == -1 ||
        get_body(&request_line, &http_request) == -1) {
                freeHttpRequest(http_request);
                free(reqcp);
                return NULL;
        }


	free(reqcp);
	return 1;
}