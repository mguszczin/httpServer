#ifndef INOTIFY_CONFIGURATION
#define INOTIFY_CONFIGURATION
#include <stdbool.h>

bool inotifyInitialize(int *wd, int *fd, char *dirPath);

#endif