#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "server.h"
#include "socket.h"

/*
 * Function that initializes a server on the given port.
 * Returns 0 on success, -1 on error.
 * Supports concurrent connections using fork() for each client.
 */

int start_server(int port)
{
	int server_socket;

	struct sockaddr_in socket_adress;
	socklen_t adresssize = sizeof(socket_adress);

	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) <0) {
		perror("Socket creation failed");
		return -1;
	}

	socket_adress.sin_addr.s_addr = INADDR_ANY;
	socket_adress.sin_family = AF_INET;
	socket_adress.sin_port = htons(port);

	if (bind(server_socket, (struct sockaddr *)&socket_adress, adresssize) < 0) {
		perror("Binding went wrong");
		close(server_socket);
		return -1;
	}
	printf("Binded to port: %d\n", port);

	int max_queue = 5;
	if (listen(server_socket, max_queue) < 0) { 
		perror("Listening failed");
		close(server_socket);
		return -1;
	}

	// make new socket for http response
	int clientsocket;
	printf("LISTENING TO PORT: %d\n", port);
	while ((clientsocket = accept(server_socket, 
                (struct sockaddr *)&socket_adress, &adresssize)) >= 0) {
		// create concurrent connection
		printf("Accepted connection\n");
                
		pid_t proces_id = fork();
		if (proces_id == 0) {
			close(server_socket);
			handle_socket(clientsocket);
                        close(clientsocket);
			exit(0);
		} else if (proces_id > 0) {
			close(clientsocket);
			continue;
		} else {
			perror("Forking went wrong");
			close(server_socket);
			return -1;
		}
	}

	close(server_socket);

	return 0;
}
