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
} http_status_code;

typedef enum {
        HDR_CACHE_CONTROL = 0,
        HDR_CONNECTION,
        HDR_LOCATION,
        HDR_SET_COOKIE,
        HDR_WWW_AUTHENTICATE,
} allowed_headers;

typedef enum {
        MIME_HTML = 0,
        MIME_HTM,
        MIME_CSS,
        MIME_JS,
        MIME_JSON,
        MIME_TXT,
        MIME_XML,

        MIME_PNG,
        MIME_JPG,
        MIME_JPEG,
        MIME_GIF,
        MIME_SVG,
        MIME_ICO,

        MIME_PDF,
        MIME_MP3,
        MIME_MP4,
        MIME_WASM,

        MIME_WOFF,
        MIME_WOFF2,
        MIME_TTF,

        MIME_DEFAULT,  // application/octet-stream (fallback)
} mime_type_e;

typedef struct http_response_t http_response_t;

mime_type_e get_content_type(char* url_path, http_response_t* http_response);

http_response_t* initialize_http_response(http_status_code status_code);

int send_http_response(http_response_t *res, int clientSocket);

void free_http_response(http_response_t *res);

int add_allowed_header(http_response_t *res, allowed_headers header, char* header_value);

int add_body(http_response_t *res, char *body, mime_type_e type);


#endif