#include <stdio.h>
#include <stdlib.h>
#include "server.h"

int main() {
    int port = 8080;

    if(start_server(port) < 0) {
        perror("directory unspecified");
        exit(EXIT_FAILURE);
    }
    
    return 0;
}