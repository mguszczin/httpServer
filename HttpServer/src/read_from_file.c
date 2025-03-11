#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "read_from_file.h"

char * getfile(char * filepath){
    FILE *file = fopen(filepath, "r");

    if(file == NULL) {
        return NULL;
    }

    // get file 
    struct stat file_status;
    if (stat(filepath, &file_status) < 0) {
        fclose(file);
        return NULL;
    }

    // allocate memory for importing body from file 
    int file_size = file_status.st_size;
    char *filecontent = malloc(file_size + 1); // +1 for null terminator

    if (filecontent == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    // read file contents
    if (fread(filecontent, 1, file_size, file) < (size_t)file_size) {
        perror("File reading error");
        free(filecontent);
        fclose(file);
        return NULL;
    }


    filecontent[file_size] = '\0';
    
    fclose(file);

    return filecontent;

}