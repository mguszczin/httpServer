#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wrap_file_to_html.h"
#include "http_response.h"

void getHtmlBodyfromFile(char * filecontent, HttpResponse *res){
    
    int size = snprintf(NULL, 0, HTTP_TEMPLATE, filecontent) + 1;
    if(size < 0) {
        perror("NULL snprintf went wrong in getHttpBodyfromFile");
        return ;
    }

    char *new_html_wrapped_file = (char *)malloc(sizeof(char) * size);
    if(snprintf(new_html_wrapped_file, size, HTTP_TEMPLATE, filecontent) < 0) {
        perror("snprintf went wrong in getHttpBodyfromFile");
        free(new_html_wrapped_file);
        return;
    }
    addBody(res, new_html_wrapped_file, HTML);
}