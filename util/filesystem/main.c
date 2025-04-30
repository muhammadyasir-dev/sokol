
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILES 100
#define MAX_FILE_NAME 50
#define MAX_FILE_CONTENT 1000

typedef struct {
    char name[MAX_FILE_NAME];
    char content[MAX_FILE_CONTENT];
} File;

File files[MAX_FILES];
int file_count = 0;

void create_file(const char *name, const char *content) {
    if (file_count >= MAX_FILES) {
        printf("Filesystem full\n");
        return;
    }
    strncpy(files[file_count].name, name, MAX_FILE_NAME);
    strncpy(files[file_count].content, content, MAX_FILE_CONTENT);
    file_count++;
}

void read_file(const char *name) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            printf("Content of %s: %s\n", name, files[i].content);
            return;
        }
    }
    printf("File not found\n");
}

int main() {
    create_file("example.txt", "Hello, World!");
    read_file("example.txt");
    return 0;
}

