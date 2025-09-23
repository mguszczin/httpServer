#ifndef BUFFER_READ
#define BUFFER_READ

extern const int DEFAULT_CHUNK;
extern const int SMALL_CHUNK;
extern const int HTTP_SMALL;
extern const int HTTP_LARGE;
extern const int INOTIFY_RECOMMENDED;

int dynamic_read(int file_descriptor, char **buffer, const int READ_CHUNK);

int static_read(int file_descriptor, char **buffer, const int BUFFER_SIZE);

#endif