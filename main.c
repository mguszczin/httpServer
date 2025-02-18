#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8080

int main() {

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

    int max_queue = 1;
    if(listen(serversocket, max_queue) < 0) {       // make socket listen to calls 
        perror("Listening failed");
        close(serversocket);
        exit(EXIT_FAILURE);
    }

    int clientsocket;
    if((clientsocket = accept(serversocket, (struct sockaddr *)&socketadress, &adresssize)) < 0) {
        perror("Can't connect with socket");
        close(serversocket);
        exit(EXIT_FAILURE);
    }

    printf("Hello World, Socket id : %d", clientsocket);

    close(serversocket);

    return 0;
}