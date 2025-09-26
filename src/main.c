#include "server/server.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define DEFAULT_PORT 8080

/*
 * Main function that initializes and starts the server.
 * By default, the server runs on port 8080, but it is allowed
 * to start on a different port if provided as a command-line argument.
 */

int validate_input(int* port, char* argv) {
        char *endptr;
        errno = 0;

        long val = strtol(argv, &endptr, 10);

        if (errno != 0) {
                perror("strtol failed");
                return 1;
        }

        if (*endptr != '\0') {
                fprintf(stderr, "Invalid port: not a number\n");
                return 1;
        }

        if (val < 1 || val > 65535) {
                fprintf(stderr, "Invalid port: must be between 1 and 65535\n");
                return 1;
        }

        *port = (int)val;
        return 0; 
}

int main(int argc, char** argv) {
        // automatic flush for output
        setbuf(stdout, NULL);

        int port = DEFAULT_PORT;

        if (argc == 2 && validate_input(&port, argv[1]) != 0) {
                return 1;
        } else if (argc > 2) {
                fprintf(stderr, "Too many arguments given\n");
                return 1;
        }

        printf("Using port: %d\n", port);
        if (start_server(port) < 0) {
                perror("Failed to start server");
                exit(EXIT_FAILURE);
        }

        return 0;
}
