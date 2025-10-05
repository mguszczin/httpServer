#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>

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

void html_extension(char* path) 
{
        size_t len = strlen(path);

        if (ends_with(path, "/") && len + 11 < PATH_MAX) {
                strcat(path, "index.html");
        } else if (len + 5 < PATH_MAX) {
                strcat(path, ".html");
        }
}

/* DECIDE IF THIS FUNCTION SHOULD SEND THE RESPONSE OR JUST PASS ERRNO */
char* get_file_from_request(char* full_path, int client_socket) 
{
        char* file;
        int cnt = 0;

        while (true) {
                file = read_from_file(full_path);

                if (file) 
                        break;

                if (errno == ENOENT && cnt == 0 || errno == EISDIR) {
                        html_extension(full_path);
                        cnt++;
                        continue;

                } else if (errno == ENOENT) {
                        send_http_error_response(HTTP_NOT_FOUND, 
                                                        client_socket);
                        return NULL;
                } else {
                        send_http_error_response(HTTP_INTERNAL_ERROR,
                                                         client_socket);
                        return NULL;
                }

        }

        return file;
}

void handle_normal_get(http_request_t* http_request, int client_socket) 
{
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s%s", RELATIVE_PATH,
                                                 http_request->path);

        if (!is_safe_path(RELATIVE_PATH, http_request->path)) {
                send_http_error_response(HTTP_FORBIDDEN, client_socket);
                return;
        }

        char* file = get_file_from_request(full_path, client_socket);
        if (!file) 
                return;

        http_response_t* response = create_http_response(200, "OK");
        if (!response) {
                free(file);
                send_http_error_response(HTTP_INTERNAL_ERROR, 
                                                        client_socket);
                return;
        }

        if(add_body(response, file, get_content_type(full_path)) == -1) {
                free(file);
                freeHttpResponse(response);
                send_http_error_response(HTTP_INTERNAL_ERROR, 
                                                        client_socket);
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
                send_http_error_response(HTTP_INTERNAL_ERROR, client_socket);
                return;
        }

        strncpy(event_path, http_request->path, path_len - suffix_len);
        event_path[path_len - suffix_len] = '\0';

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s%s", RELATIVE_PATH, 
                                                                event_path);

        if (!is_safe_path(RELATIVE_PATH, event_path)) {
                send_http_error_response(HTTP_FORBIDDEN, client_socket);
                free(event_path);
                return;
        }


        free(event_path);
}

#define INOTIFY_BUF_LEN 64

void handle_event_request(char* path, int client_socket) 
{
        int watch_descriptor, file_descriptor;
        if (!inotifyInitialize(&watch_descriptor, &file_descriptor, path)) {
                send_http_error_response(HTTP_INTERNAL_ERROR, client_socket);
                return;
        }

        int epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
                fprintf(stderr, "epoll_create1 failed");
                send_http_error_response(HTTP_INTERNAL_ERROR, client_socket);

                inotify_rm_watch(file_descriptor, watch_descriptor);
                close(file_descriptor);
                return;
        }

        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = file_descriptor;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, file_descriptor, &event) == -1) {
                fprintf(stderr, "epoll_ctl failed");
                send_http_error_response(HTTP_INTERNAL_ERROR, client_socket);

                close(epoll_fd);
                inotify_rm_watch(file_descriptor, watch_descriptor);
                close(file_descriptor);
                return;
        }

        struct epoll_event events[1];
        char read_buffer[INOTIFY_BUF_LEN];

        while (true) {
                int n = epoll_wait(epoll_fd, events, 1, -1);
                if (n == -1) {
                        fprintf(stderr, "epoll_wait failed");
                        send_http_error_response(HTTP_INTERNAL_ERROR, client_socket);
                        break;
                }

                read(events[0].data.fd, read_buffer, INOTIFY_BUF_LEN);
                // send sse response
                if (send_http_sse_response_message("File update", client_socket) == -1) {
                        fprintf(stderr, "Failed to send SSE message\n");
                        break;
                }
        }

        inotify_rm_watch(file_descriptor, watch_descriptor);
        close(file_descriptor);
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
