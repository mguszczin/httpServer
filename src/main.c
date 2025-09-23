#include "server.h"
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_PORT = 8080

int main(int arg, char **argv)
{
	setbuf(stdout, NULL);

	if (start_server(port) < 0) {
		perror("directory unspecified");
		exit(EXIT_FAILURE);
	}

	return 0;
}