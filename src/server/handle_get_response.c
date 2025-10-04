#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

#include "file_reader/read_from_file.h"
#include "server/handle_get_response.h"
#include "server/http_response.h"
#include "server/http_request.h"

#ifndef PATH_MAX
#define PATH_MAX 4096 
#endif

#define EVENTS_SUFFIX ".event"

/* SHOULD BE DEFINED IN GLOBAL CONFIG IN FUTURE*/
#define RELATIVE_PATH "../../html/site"

enum get_request_type {
        NORMAL,
        EVENT
};

bool ends_with(const char *str, const char *suffix) 
{
        if (!str || !suffix)
                return false;

        size_t lenstr = strlen(str);
        size_t lensuffix = strlen(suffix);

        if (lensuffix > lenstr)
                return false;

        return strcmp(str + lenstr - lensuffix, suffix) == 0;
}

bool is_safe_path(const char *base, const char *path) {
        char resolved_base[PATH_MAX];
        char resolved_path[PATH_MAX];

        if (!realpath(base, resolved_base)) return false;

        char temp[PATH_MAX];
        snprintf(temp, sizeof(temp), "%s/%s", base, path);

        if (!realpath(temp, resolved_path)) return false;

        return strncmp(resolved_base, resolved_path, strlen(resolved_base)) == 0;
}


void handle_normal_get(http_request_t* http_request, int client_socket, 
                        enum get_request_type request_type) 
{
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s%s", RELATIVE_PATH, http_request->path);

        if (!is_safe_path(RELATIVE_PATH, http_request->path)) {
                // Return 403 Forbidden
                return;
        }

        char* file = read_from_file(full_path);
        if (file == NULL) {
                if (errno == ENOMEM) {
                        // Return 500 Internal Server Error
                } else {
                        // Return 404 Not Found
                }
                return;
        }

        http_response_t* response = create_http_response(200, "OK");
        if (!response) {
                free(file);
                // Return 500 Internal Server Error
                return;
        }

        if(add_body(response, file, get_content_type(full_path)) == -1) {
                free(file);
                freeHttpResponse(response);
                // Return 500 Internal Server Error
                return;
        }

        send_http_response(client_socket, response);

        freeHttpResponse(response);
        free_file_buffer(file);
}

void handle_event(http_request_t* http_request, int client_socket) 
{
        int path_len = strlen(http_request->path);
        int suffix_len = strlen(EVENTS_SUFFIX);

        char* event_path = malloc(sizeof(char) * (path_len - suffix_len + 1));
        if (!event_path) {
                // Return 500 Internal Server Error
                return;
        }

        strncpy(event_path, http_request->path, path_len - suffix_len);
        event_path[path_len - suffix_len] = '\0';

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s%s", RELATIVE_PATH, event_path);

        if (!is_safe_path(RELATIVE_PATH, event_path)) {
                // Return 403 Forbidden
                free(event_path);
                return;
        }


        free(event_path);
}

/*
* Requests are devided into two categories:
* 1. Normal GET requests - serve static files
* 2. Event GET requests - serve events (long polling)
*/
http_response_t* handle_get_request(http_request_t* http_request, int client_socket) 
{
        
        if (ends_with(http_request->path, EVENTS_SUFFIX)) 
                handle_event(http_request, client_socket);
        else 
                handle_normal_get(http_request, client_socket);
        
}
