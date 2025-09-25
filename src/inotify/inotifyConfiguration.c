#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <unistd.h>

#include "inotify/inotifyConfiguration.h"

bool inotifyInitialize(int *wd, int *fd, char *dirPath)
{
	// Initialize inotify
	int inotify_fd = inotify_init();
	if (inotify_fd < 0) {
		perror("inotify failed to initialize");
		return false;
	}
	// Set inotify to watch directory
	int watch = inotify_add_watch(inotify_fd, dirPath, IN_MODIFY);
	if (watch < 0) {
		printf("i am true then\n");
		perror("inotify_add_watch");
		close(inotify_fd);
		return false;
	}
	*wd = watch;
	*fd = inotify_fd;
	return true;
}