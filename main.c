#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h> 
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 256

// gzip , files

static const char *dirPath = NULL;

void return_file(char **response, char *RequestLine, char *path) {

    const char *header_template =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s";
    
    const char *not_found_response = 
    "HTTP/1.1 404 Not Found\r\n\r\n";

    if(dirPath == NULL) {
        *response = strdup(not_found_response); // Allocate memory for response
        return;
    }
    char *filename = path + 5;
    char *filepath = (char *)malloc(strlen(dirPath) + strlen(filename) + 10);
    strcat(filepath, dirPath);
    strcat(filepath, filename);

    FILE *file = fopen(filepath, "r");

    if (file == NULL) {
        *response = strdup(not_found_response); // Allocate memory for response
        return;
    }

    // get file 
    struct stat file_status;
    if (stat(filepath, &file_status) < 0) {
        fclose(file);
        *response = strdup(not_found_response);
        return;
    }

    // allocate memory for importing body from file 
    int body_size = file_status.st_size;
    char *body = malloc(body_size + 1); // +1 for null terminator
    if (body == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        *response = strdup(not_found_response);
        return;
    }

    // read file contents
    if (fread(body, 1, body_size, file) < body_size) {
        perror("File reading error");
        free(body);
        fclose(file);
        *response = strdup(not_found_response);
        return;
    }
    body[body_size] = '\0';

    // allocate the maximum memory for response
    int response_size = strlen(header_template) + body_size + 50;
    *response = malloc(response_size);
    if (*response == NULL) {
        perror("Memory allocation failed");
        free(body);
        fclose(file);
        return;
    }

    // write respond to buffer
    if(snprintf(*response, response_size, header_template, body_size, body) < 0) {
        perror("echo went wrong");
    }

    free(body);
    fclose(file);
}

void add_echo(char **response, char *RequestLine, char *path) {

    char *body = path + 6;

    const char *header_template =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s";

    // compute maximal buffer size
     int max_size = 200 + strlen(body);

    *response = (char *)malloc(max_size * sizeof(char));
    if (!*response) return;

    if(snprintf(*response, max_size, header_template, (int)strlen(body), body) < 0) {
        perror("echo went wrong");
    }
}


void handle_socket(int clientsocket) {
    // read http request
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(clientsocket, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("read failed");
        close(clientsocket);
        return;
    }

    // get basic responses
    char responseOk[] = "HTTP/1.1 200 OK\r\n\r\n";
    char responseNotFound[] = "HTTP/1.1 404 Not Found\r\n\r\n";
    char *response;

    char *RequestLine = strtok(buffer, "\r\n");

    char *method = strtok(RequestLine, " ");
    char *path = strtok(NULL, " ");
    char *protocol = strtok(NULL, " ");

    bool successful;

    if(strncmp(path, "/", strlen(path)) == 0) response = strdup(responseOk);
    else if(strncmp(path, "/echo/", 6) == 0) add_echo(&response, RequestLine, path);
    else if(strncmp(path, "/file/", 6) == 0) return_file(&response, RequestLine, path);
    else response = strdup(responseNotFound);

    if(send(clientsocket, response, strlen(response),0) < 0){
        perror("write failed");
    }

    close(clientsocket);
    free(response);
}

void get_directory_path(int argc, char *argv[]) {
    for(int i = 1; i < argc; i++) {

        if(strcmp(argv[i], "--directory") == 0) {
            if((i + 1) != argc) {
                dirPath = argv[i + 1];
                break;
            }
            else {
                perror("directory unspecified");
                exit(EXIT_FAILURE);
            }
        }

    }
}

int main(int argc, char *argv[]) {

    get_directory_path(argc, argv);

    int serversocket;

    struct sockaddr_in socketadress;
    int adresssize = sizeof(socketadress);
    
    if((serversocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {         // creating socket for tcp connections
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    socketadress.sin_addr.s_addr = INADDR_ANY;    
    socketadress.sin_family = AF_INET;
    socketadress.sin_port = htons(PORT); 

    if(bind(serversocket, (struct sockaddr *)&socketadress, adresssize) < 0) {       // bind adress to a socket 
        perror("Binding went wrong");
        close(serversocket);
        exit(EXIT_FAILURE);
    }

    int max_queue = 5;
    if(listen(serversocket, max_queue) < 0) {       // make socket listen to calls 
        perror("Listening failed");
        close(serversocket);
        exit(EXIT_FAILURE);
    }

    // make new socket for http response
    int clientsocket;                             
    while((clientsocket = accept(serversocket, (struct sockaddr *)&socketadress, &adresssize)) >= 0) {
        // create concurrent connection
        pid_t procesid = fork();
        
        if(procesid == 0) {
            close(serversocket);
            handle_socket(clientsocket);
            exit(0);
        }else if(procesid > 0) {
            close(clientsocket);
            continue;
        }else {
            perror("Forking went wrong");
            break;
        }
    }

    close(serversocket);

    return 0;
}