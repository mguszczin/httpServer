#ifndef HTTP_FILE_MANAGMENT
#define HTTP_FILE_MANAGMENT

#include "http_response.h"

static const char *HTTP_TEMPLATE =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "    <meta charset=\"UTF-8\">\n"
    "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "    <title>Simple Page</title>\n"
    "</head>\n"
    "<body>\n"
    "    <h1>%s</h1>\n"
    "</body>\n"
    "</html>\n";


void getHtmlBodyfromFile(char *filePath, HttpResponse *res);


#endif