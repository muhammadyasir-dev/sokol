#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Define a simple inode structure
typedef struct {
    char filename[256];
    uint32_t size;
    uint32_t block_start;
} inode_t;

// Define a simple filesystem structure
typedef struct {
    inode_t *inodes;
    uint32_t inode_count;
} filesystem_t;

// Initialize the filesystem
filesystem_t* init_filesystem() {
    filesystem_t *fs = (filesystem_t*)malloc(sizeof(filesystem_t));
    fs->inodes = (inode_t*)malloc(sizeof(inode_t) * 100); // Allocate space for 100 inodes
    fs->inode_count = 0;
    return fs;
}

// Create a new file
void create_file(filesystem_t *fs, const char *filename, uint32_t size, uint32_t block_start) {
    if (fs->inode_count < 100) {
        inode_t *inode = &fs->inodes[fs->inode_count++];
        strncpy(inode->filename, filename, 256);
        inode->size = size;
        inode->block_start = block_start;
    }
}

int main() {
    filesystem_t *fs = init_filesystem();
    create_file(fs, "test.txt", 1024, 0);
    // Add more filesystem operations here
    free(fs->inodes);
    free(fs);
    return 0;
}

