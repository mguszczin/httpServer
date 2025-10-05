#ifndef HTTP_ERROR_RESPONSE_H
#define HTTP_ERROR_RESPONSE_H

#include "http_response.h"

void send_http_error_response(http_status_code status_code, int client_socket);

#endif