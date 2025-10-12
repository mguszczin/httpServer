#ifndef HANDLE_GET_REQUEST_H
#define HANDLE_GET_REQUEST_H

#include <server/http_response.h>
#include <server/http_request.h>

void handle_get_request(http_request_t* http_request, int client_socket);

#endif