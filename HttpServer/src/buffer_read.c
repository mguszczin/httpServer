
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "buffer_read.h"

const int DEFAULT_CHUNK = 512;
const int SMALL_CHUNK = 1;
const int HTTP_SMALL = 256;
const int HTTP_LARGE = 16 * 1024 * 1024;  // 16MB
const int INOTIFY_RECOMMENDED = 16 * 1024;  // 16KB

int static_read(int file_descriptor, char **buffer, const int BUFFER_SIZE) {
    *buffer = NULL;
    if (file_descriptor < 0) {
        perror("Invalid file descriptor");
        return -1;
    }
    
    // generate new buffer for read
    char* newBuffer = malloc(sizeof(char) * BUFFER_SIZE);
    if(newBuffer == NULL) {
        perror("malloc failed");
        return -1;
    }
    
    int bytes_read = read(file_descriptor, newBuffer, BUFFER_SIZE);
    
    // check if read worked
    if(bytes_read < 0) {
        free(newBuffer);
        perror("reading went wrong");
        return -1;
    }

    // we want to make sure that we read everything
    if(bytes_read == BUFFER_SIZE) {
        perror("too much to read");
        return -1;
    }

    // ending with null-termination
    newBuffer[bytes_read] = '\0';
    *buffer = newBuffer;

    return bytes_read;
}

int dynamic_read(int file_descriptor, char **buffer, const int READ_CHUNK) {
    *buffer = NULL;
    if (file_descriptor < 0) {
        perror("Invalid file descriptor");
        return -1;
    }

    int position = 0;
    int capacity = READ_CHUNK;
    *buffer = malloc(capacity);
    if (!*buffer) {
        perror("malloc failed");
        return -1;
    }

    ssize_t bytes_read;
    while (1) {
        // read chunks of bytes
        bytes_read = read(file_descriptor, (*buffer) + position, READ_CHUNK);
        if (bytes_read > 0) {
            position += bytes_read;

            // expand the buffer if needed
            if (position + READ_CHUNK > capacity) {
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
            // end-of-file reached
            break;
        } else { // bytes_read == -1, an error occurred
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                perror("read error");
                free(*buffer);
                *buffer = NULL;
                return -1;
            }
        }
    }

    // null-terminate the buffer
    (*buffer)[position] = '\0';
    return position;
}
