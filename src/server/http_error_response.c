#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server/http_error_response.h"
#include "http_response.h"
#include "http_error_response.h"

static const char *error_template =
        "<html>\n"
        "  <head>\n"
        "    <title>%d %s</title>\n"
        "    <style>\n"
        "      body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }\n"
        "      h1 { font-size: 50px; color: #cc0000; }\n"
        "      p { font-size: 20px; color: #333333; }\n"
        "    </style>\n"
        "  </head>\n"
        "  <body>\n"
        "    <h1>%d</h1>\n"
        "    <p>%s</p>\n"
        "  </body>\n"
        "</html>\n";

static const char *status_mappings[] = {
	[400] = "Bad Request",
        [403] = "Forbidden",
        [404] = "Not Found",
	[500] = "Internal Server Error"
        };


static const char* get_status_message(http_status_code status_code) {
        if (status_code < 400 || status_code > 599 || status_mappings[status_code] == NULL) {
                return "Undefined Error";
        }
        return status_mappings[status_code];
}

static char* generate_body(http_status_code status_code) {
        const char* status_message = get_status_message(status_code);
        int body_size = snprintf(NULL, 0, error_template, status_code, status_message, status_code, status_message) + 1;

        char* body = (char*)malloc(body_size * sizeof(char));
        if (!body) {
                perror("Malloc for error body failed");
                return NULL;
        }

        snprintf(body, body_size, error_template, status_code, status_message, status_code, status_message);
        return body;
}

void send_http_error_response(http_status_code status_code, int client_socket)
{
        if (status_code < 400 || status_code > 599) {
                fprintf(stderr, "Invalid status code for error response: %d\n", status_code);
                return;
        }
        
        http_response_t *response = initialize_http_response(status_code);
        if (!response) {
                fprintf(stderr, "Failed to create HTTP error response\n");
                return;
        }

        char* body = generate_body(status_code);
        if (!body) {
                free_http_response(response);
                return;
        }

        if (add_body(response, body, MIME_HTML) == -1) {
                fprintf(stderr, "Failed to add body to HTTP error response\n");
                free_http_response(response);
                free(body);
                return;
        }

        if (send_http_response(response, client_socket) == -1) {
                fprintf(stderr, "Failed to send HTTP error response\n");
        }

        free(body);
        free_http_response(response);
}