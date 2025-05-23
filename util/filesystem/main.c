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

static File files[MAX_FILES];
static int file_count = 0;

void create_file(const char *name, const char *content) {
    if (file_count >= MAX_FILES) {
        printf("Error: filesystem full\n");
        return;
    }
    // Prevent duplicate names
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            printf("Error: file '%s' already exists\n", name);
            return;
        }
    }
    strncpy(files[file_count].name, name, MAX_FILE_NAME-1);
    files[file_count].name[MAX_FILE_NAME-1] = '\0';
    strncpy(files[file_count].content, content, MAX_FILE_CONTENT-1);
    files[file_count].content[MAX_FILE_CONTENT-1] = '\0';
    file_count++;
    printf("File '%s' created.\n", name);
}

void read_file(const char *name) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            printf("---- %s ----\n%s\n", name, files[i].content);
            return;
        }
    }
    printf("Error: file '%s' not found\n", name);
}

void list_files() {
    if (file_count == 0) {
        printf("No files in filesystem.\n");
        return;
    }
    printf("Files:\n");
    for (int i = 0; i < file_count; i++) {
        printf("  %s\n", files[i].name);
    }
}

void delete_file(const char *name) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            // Move last file into this slot
            files[i] = files[file_count-1];
            file_count--;
            printf("File '%s' deleted.\n", name);
            return;
        }
    }
    printf("Error: file '%s' not found\n", name);
}

void update_file(const char *name, const char *new_content) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            strncpy(files[i].content, new_content, MAX_FILE_CONTENT-1);
            files[i].content[MAX_FILE_CONTENT-1] = '\0';
            printf("File '%s' updated.\n", name);
            return;
        }
    }
    printf("Error: file '%s' not found\n", name);
}

void append_file(const char *name, const char *more_content) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            strncat(files[i].content, more_content, 
                    MAX_FILE_CONTENT - strlen(files[i].content) - 1);
            printf("Appended to '%s'.\n", name);
            return;
        }
    }
    printf("Error: file '%s' not found\n", name);
}

void print_menu() {
    puts("\nIn-Memory Filesystem");
    puts("1) Create file");
    puts("2) Read file");
    puts("3) List files");
    puts("4) Delete file");
    puts("5) Update file");
    puts("6) Append to file");
    puts("0) Exit");
    printf("Choose an option: ");
}

int main(void) {
    int choice;
    char name[MAX_FILE_NAME];
    char content[MAX_FILE_CONTENT];

    do {
        print_menu();
        if (scanf("%d", &choice) != 1) {
            // flush invalid input
            while (getchar() != '\n'); 
            continue;
        }
        getchar(); // consume newline

        switch (choice) {
            case 1:
                printf("Enter filename: ");
                fgets(name, sizeof name, stdin);
                name[strcspn(name, "\n")] = '\0';
                printf("Enter content: ");
                fgets(content, sizeof content, stdin);
                content[strcspn(content, "\n")] = '\0';
                create_file(name, content);
                break;

            case 2:
                printf("Enter filename: ");
                fgets(name, sizeof name, stdin);
                name[strcspn(name, "\n")] = '\0';
                read_file(name);
                break;

            case 3:
                list_files();
                break;

            case 4:
                printf("Enter filename: ");
                fgets(name, sizeof name, stdin);
                name[strcspn(name, "\n")] = '\0';
                delete_file(name);
                break;

            case 5:
                printf("Enter filename: ");
                fgets(name, sizeof name, stdin);
                name[strcspn(name, "\n")] = '\0';
                printf("Enter new content: ");
                fgets(content, sizeof content, stdin);
                content[strcspn(content, "\n")] = '\0';
                update_file(name, content);
                break;

            case 6:
                printf("Enter filename: ");
                fgets(name, sizeof name, stdin);
                name[strcspn(name, "\n")] = '\0';
                printf("Enter additional content: ");
                fgets(content, sizeof content, stdin);
                content[strcspn(content, "\n")] = '\0';
                append_file(name, content);
                break;

            case 0:
                puts("Exiting.");
                break;

            default:
                puts("Invalid choice, try again.");
        }
    } while (choice != 0);

    return 0;
}

