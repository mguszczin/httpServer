#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <time.h> 

#include "server/http_response.h"

typedef struct {
	int status_code;
	char *status_message;
} start_line_t;

static const char *status_mappings[] = {
        [200] = "OK",
	[201] = "Created",
	[400] = "Bad Request",
        [404] = "Not Found",
	[500] = "Internal Server Error"
        };

typedef struct {
        size_t header_index;
        char* value;
        bool allowed;
} header_t;

struct http_response_t {
	start_line_t starting_line;

	header_t* headers;
	int header_count;

	char *body;
	int body_size;
};

typedef struct {
        const char *ext;
        const char *mime;
} mime_map;

static mime_map mime_types[] = {
        { "html",    "text/html; charset=UTF-8" },
        { "htm",     "text/html; charset=UTF-8" },
        { "css",     "text/css; charset=UTF-8" },
        { "js",      "application/javascript; charset=UTF-8" },
        { "json",    "application/json; charset=UTF-8" },
        { "txt",     "text/plain; charset=UTF-8" },
        { "xml",     "application/xml; charset=UTF-8" },

        { "png",     "image/png" },
        { "jpg",     "image/jpeg" },
        { "jpeg",    "image/jpeg" },
        { "gif",     "image/gif" },
        { "svg",     "image/svg+xml" },
        { "ico",     "image/x-icon" },

        { "pdf",     "application/pdf" },
        { "mp3",     "audio/mpeg" },
        { "mp4",     "video/mp4" },
        { "wasm",    "application/wasm" },

        { "woff",    "font/woff" },
        { "woff2",   "font/woff2" },
        { "ttf",     "font/ttf" },

        { NULL,      "application/octet-stream" } 
};

typedef enum {
        HDR_CONTENT_LENGTH = 0,
        HDR_CONTENT_TYPE,
        HDR_DATE
} automatic_header;

static const char* automatic_header_names[] = {
        "Content-Length",      
        "Content-Type",  
        "Date",     
};

static const char* allowed_header_names[] = {
        "Cache-Control",       
        "Connection",                                                   
        "Location",            
        "Set-Cookie",         
        "WWW-Authenticate",   
};

static const char* find_extension(const char* path) 
{
        const char* dot = strrchr(path, '.');  
        if (!dot || dot == path) return NULL; 
        return dot + 1;
}

static mime_type_e lookup_mime_type(const char* ext) {
        if (!ext)
                return MIME_DEFAULT; 
        
        for (mime_type_e i = 0; i != MIME_DEFAULT; i++) {
                if (strcmp(ext, mime_types[i].ext) == 0) {
                        return i;
                }
        }
        return MIME_DEFAULT; 
}

mime_type_e get_content_type(char* url_path) 
{
        const char *ext = find_extension(url_path);
        mime_type_e mime = lookup_mime_type(ext);

        return mime;
}

int set_start_line(http_response_t *res, http_status_code status_code)
{

	char *status_message;

	if (status_code < 0 || status_mappings[status_code] == NULL) {
                fprintf(stderr, "Unknown status code");
                return -1;
	}

        status_message = strdup(status_mappings[status_code]);
        if (!status_message) {
                fprintf(stderr, "Out of memory");
                return -1;
        }

	res->starting_line = (start_line_t){.status_code = status_code,
					    .status_message = status_message};
        return 0;
}

http_response_t* initialize_http_response(http_status_code status_code)
{
        http_response_t* res = malloc(sizeof(http_response_t));
        if (!res) {
                fprintf(stderr, "No memory for http response");
                return NULL;
        }

	if (set_start_line(res, status_code) < 0) {
                free(res);
                return NULL;
        }

	res->header_count = 0;
	res->headers = NULL;

	res->body = strdup("");
	res->body_size = 0;

        if (!res->body) {
                free(res->starting_line.status_message);
                free(res->body);
                free(res);
                return NULL;
        }
        return res;
}

static int add_header(http_response_t *res, int header_index, const char* header_value, bool allowed) 
{
        if (!res || !header_value) {
		fprintf(stderr, "Error: NULL input to add_header\n");
		return -1;
	}

        char* value_copy = strdup(header_value);
        if (!value_copy) {
                fprintf(stderr, "Wrong Memory allocation");
                return -1;
        }

	header_t* tmp =
	    realloc(res->headers, (res->header_count + 1) * sizeof(header_t));
	if (tmp == NULL) {
                free(value_copy);
		perror("Malloc went wrong while adding a header");
		return -1;
	}

	res->headers = tmp;
	res->headers[res->header_count] = (header_t){
                .allowed = allowed,
                .header_index = header_index,
                .value = value_copy
        };
	res->header_count++;
        return 0;
}

int add_allowed_header(http_response_t *res, allowed_headers header, const char* header_value)
{
        return add_header(res, header, header_value, true);
}

static int add_automatic_header(http_response_t *res, automatic_header header, const char* header_value)
{
        return add_header(res, header, header_value, false);
}

int get_start_line_response(http_response_t *res, char **start_line)
{
	int start_line_size = snprintf(NULL, 0, "HTTP/1.1 %d %s\r\n",
				      res->starting_line.status_code,
				      res->starting_line.status_message)
                                      +1;

	*start_line = (char *)malloc(start_line_size * sizeof(char));
	if (*start_line == NULL) {
		perror("Malloc of Starting Line went wrong");
		return -1;
	}

	snprintf(*start_line, start_line_size, "HTTP/1.1 %d %s\r\n",
			      res->starting_line.status_code,
			      res->starting_line.status_message);
	return 0;
}

int add_automatic_headers(http_response_t* res) 
{
        char date[50];

        time_t now = time(NULL);
        struct tm tm;
        gmtime_r(&now, &tm);
        strftime(date, 50, "%a, %d %b %Y %H:%M:%S GMT", &tm);

        return add_automatic_header(res, HDR_DATE, date);
}

int get_response_header_lenght(header_t header) {
        return header.allowed ? strlen(allowed_header_names[header.header_index]) :
                                strlen(automatic_header_names[header.header_index]);
}

void copy_header(char** header_line, header_t header) {
        int len = get_response_header_lenght(header);
                
        (header.allowed) ? 
        memcpy(*header_line, allowed_header_names[header.header_index], len) : 
        memcpy(*header_line, automatic_header_names[header.header_index], len);
        *header_line += len;

        memcpy(*header_line, ": ", strlen(": "));
        *header_line += strlen(": ");

        memcpy(*header_line, header.value, strlen(header.value));
        *header_line += strlen(header.value);

        memcpy(*header_line, "\r\n", strlen("\r\n"));
        *header_line += strlen("\r\n");
}

void print_response_crlf_nicely(const char *response) {
    size_t i = 0;
    int line_num = 1;

    printf("=== HTTP Response Visualization ===\n");
    printf("Line %d: ", line_num);

    while (i < strlen(response)) {
        if (response[i] == '\r') {
            if (i + 1 < strlen(response) && response[i + 1] == '\n') {
                printf("<CRLF>\n");
                i++; // skip '\n'
                line_num++;
                if (i + 1 < strlen(response)) {
                    printf("Line %d: ", line_num);
                }
            } else {
                printf("\\r");
            }
        } else if (response[i] == '\n') {
            printf("\\n\n");
            line_num++;
            if (i + 1 < strlen(response)) {
                printf("Line %d: ", line_num);
            }
        } else {
            putchar(response[i]);
        }
        i++;
    }
    printf("\n=== End of Response ===\n");
}

int get_headers_response(http_response_t *res, char **headers)
{       
        if (add_automatic_headers(res) < 0) 
                return -1;
        
        char* crlf = "\r\n";
	int crlf_len = strlen("\r\n");
        int header_len = 0;
	for (int i = 0; i < res->header_count; i++) {
                header_len += get_response_header_lenght(res->headers[i]) +
                4 + strlen(res->headers[i].value);                                     // crlf + ": " that's why + 4
        }

        header_len += crlf_len + 1;                                        /*final crlf and +1 for null terminator*/

        char* header_line = malloc(sizeof(char) * (header_len));
        if (!header_line) {
                fprintf(stderr, "failed to allocate header line");
                return -1;
        }

        *headers = header_line;

        for (int i = 0; i < res->header_count; i++) {
               copy_header(&header_line, res->headers[i]);
        }

        memcpy(header_line, crlf, crlf_len);
        header_line += crlf_len;

        *header_line = '\0';
        return 0;
}

int get_body_response(http_response_t *res, char **body)
{

	// calculate body size - +1 for null terminator
	int body_size = strlen(res->body) + 1;

	*body = (char *)malloc(body_size * sizeof(char));
	if (!*body) {
		perror("Malloc of Body went wrong");
		return -1;
	}

	snprintf(*body, body_size, "%s", res->body);
        return 0;
}

// function uses get_start_line_response, get_Headers_response and
// get_Body_response
int send_http_response(http_response_t *res, int clientSocket)
{
	char *start_line = NULL;
	char *headers = NULL;
	char *body = NULL;

	// check i Response exists
	if (res->starting_line.status_code == -1)
		return -1;

	// get all the needed responses
	if (get_start_line_response(res, &start_line) == -1 ||
	    get_headers_response(res, &headers) == -1 ||
	    get_body_response(res, &body) == -1) {
                free(start_line);
                free(headers);
                free(body);
                return -1;
        }

	char *response = NULL;
	int response_size =
	    snprintf(NULL, 0, "%s%s%s", start_line, headers, body) +
	    1; // +1 for null terminator

	response = (char *)malloc(response_size * sizeof(char));
	if (response == NULL) {
                free(start_line);
                free(headers);
                free(body);
		perror("Malloc of http_response_t went wrong");
		return -1;
	}

	// merge all three parts
	snprintf(response, response_size, "%s%s%s", start_line, headers, body);
        print_response_crlf_nicely(response);
	// send http response to clientsocket
        size_t total = 0;
        size_t to_write = strlen(response);
        while (total < to_write) {
                ssize_t sent = write(clientSocket, response + total, to_write - total);
                if (sent <= 0) {
                        perror("write failed");
                        break;
                }
                total += sent;
        }
        printf("val : %ld\nresponse size : %ld \n", total, strlen(response));
        printf("body size: %d\nstrlen body:%ld\n", res->body_size, strlen(body));
        free(start_line);
        free(headers);
        free(body);
	free(response);
	return 0;
}

void free_http_response(http_response_t *res)
{
	free(res->starting_line.status_message);

        for(int i = 0; i < res->header_count; i++) {
                free(res->headers[i].value);
        }
        free(res->headers);

	free(res->body);
	free(res);
}

// remember content type and lenght 
/*
TO DO: change function so that you can add a few headers and deal with memory error
*/
int add_body(http_response_t *res, const char *body, mime_type_e type)
{
        if (!res || !body) {
                fprintf(stderr, "Null pointers given");
                return -1;
        }
        res->body_size = strlen(body);

        char size[13];
        snprintf(size, sizeof(size), "%d", res->body_size);

        if (add_automatic_header(res, HDR_CONTENT_LENGTH, size) < 0) {
                res->body_size = 0;
                return -1;
        }

        if (add_automatic_header(res, HDR_CONTENT_TYPE, mime_types[type].mime) < 0) {
                res->body_size = 0;
                return -1;
        }

	res->body = strdup(body);
        if(!body) {
                fprintf(stderr, "Failed to copy body");
                res->body_size = 0;
                return -1;
        }
        

        return 0;
}