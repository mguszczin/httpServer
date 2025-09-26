#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>

#include "server/http_response.h"
#include "server/http_request.h"

#ifndef PATH_MAX
#define PATH_MAX 4096 
#endif

#define RELATIVE_PATH "../../html/site"

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

/*
http_response_t* handle_get_request(http_request_t* http_request) 
{
        
        if (ends_with(http_request->path, ".event")) 
                handle_event(http_request);
        else 
                handle_normal_get(http_request);
        
}
*/