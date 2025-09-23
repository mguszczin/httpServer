#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_response.h"
#include "wrap_file_to_html.h"

static const char *HTTP_TEMPLATE =
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "    <meta charset=\"UTF-8\">\n"
    "    <meta name=\"viewport\" content=\"width=device-width, "
    "initial-scale=1.0\">\n"
    "    <title>Simple Page</title>\n"
    "</head>\n"
    "<body>\n"
    "    <h1>%s</h1>\n"
    "</body>\n"
    "</html>\n";

// function should be improved a little bit
// now it will break if there will be special signs like \ or %
void getHtmlBodyfromFile(char *filecontent, HttpResponse *res)
{

	int size = snprintf(NULL, 0, HTTP_TEMPLATE, filecontent) + 1;
	if (size < 0) {
		perror("NULL snprintf went wrong in getHttpBodyfromFile");
		return;
	}

	char *new_html_wrapped_file = (char *)malloc(sizeof(char) * size);
	if (snprintf(new_html_wrapped_file, size, HTTP_TEMPLATE, filecontent) <
	    0) {
		perror("snprintf went wrong in getHttpBodyfromFile");
		free(new_html_wrapped_file);
		return;
	}
	addBody(res, new_html_wrapped_file, HTML);
}