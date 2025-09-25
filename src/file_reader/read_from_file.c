#include "file_reader/read_from_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

/*
 * Reads the entire contents of a file into a dynamically allocated buffer.
 * Returns a pointer to the buffer (caller must free it) on success, or NULL on failure.
 * If the path points to a directory, NULL is returned.
 */
char *getfile(char *filepath)
{
	FILE *file = fopen(filepath, "r");
	if (file == NULL) {
		return NULL;
	}

	struct stat st;
	fstat(fileno(file), &st);
	if (fstat(fileno(file), &st) < 0) {
		fclose(file);
		return NULL;
	}

        if (S_ISDIR(st.st_mode)) {
                fclose(file);
                return NULL;
        }

	// allocate memory for importing body from file
	size_t file_size = st.st_size;
	char *filecontent = malloc(file_size + 1); // +1 for null terminator

	if (filecontent == NULL) {
		perror("Memory allocation failed");
		fclose(file);
		return NULL;
	}

	size_t read_bytes = fread(filecontent, 1, file_size, file);
        if (read_bytes != file_size) {
                perror("File reading error");
                free(filecontent);
                fclose(file);
                return NULL;
        }

	filecontent[file_size] = '\0';

	fclose(file);

	return filecontent;
}