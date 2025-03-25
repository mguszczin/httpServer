
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "buffer_read.h"

int dynamic_read(int file_descriptor, char **buffer) {
    *buffer = NULL;
    if (file_descriptor < 0) {
        perror("Invalid file descriptor");
        return -1;
    }

    int position = 0;
    int capacity = 2;  // Start with a small buffer but room for '\0'
    *buffer = malloc(capacity);
    if (!*buffer) {
        perror("malloc failed");
        return -1;
    }

    ssize_t bytes_read;
    while (1) {
        bytes_read = read(file_descriptor, (*buffer) + position, 1);
        if (bytes_read > 0) {
            // Print the current character for debugging
            position++;

            // Ensure there's enough space for the next character and the null terminator
            if (position >= capacity - 1) {
                capacity *= 2;
                char *temp = realloc(*buffer, capacity);
                if (!temp) {
                    perror("realloc failed");
                    free(*buffer);
                    *buffer = NULL;
                    return -1;
                }
                *buffer = temp;
            }
        } else if (bytes_read == 0) {
            // End-of-file reached
            break;
        } else { // bytes_read == -1, an error occurred
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if(errno == EAGAIN) printf("EAGAIN\n");
                else printf("EWOULDBLOCK\n");
               break;
            } else {
                perror("read error");
                free(*buffer);
                *buffer = NULL;
                return -1;
            }
        }
    }

    // Null-terminate the buffer
    (*buffer)[position] = '\0';
    return position;
}