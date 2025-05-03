#include <stdint.h>

#define SECTOR_SIZE 512
#define MAX_FILES   128
#define FILE_TABLE_SECTOR 1

#define PORT_DATA       0x1F0
#define PORT_SECCOUNT   0x1F2
#define PORT_LBA_LOW    0x1F3
#define PORT_LBA_MID    0x1F4
#define PORT_LBA_HIGH   0x1F5
#define PORT_DRIVE      0x1F6
#define PORT_COMMAND    0x1F7
#define PORT_STATUS     0x1F7

#define CMD_READ_SECTOR 0x20
#define CMD_WRITE_SECTOR 0x30

#define PERM_READ  0x1
#define PERM_WRITE 0x2

typedef struct {
    char name[16];
    uint32_t start_sector;
    uint32_t size;
    uint8_t permissions;
} __attribute__((packed)) FileEntry;

FileEntry file_table[MAX_FILES];

// Low-level I/O
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void ata_wait() {
    while (inb(PORT_STATUS) & 0x80); // BSY bit
}

void ata_read_sector(uint32_t lba, uint8_t *buf) {
    ata_wait();
    outb(PORT_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(PORT_SECCOUNT, 1);
    outb(PORT_LBA_LOW,  lba & 0xFF);
    outb(PORT_LBA_MID,  (lba >> 8) & 0xFF);
    outb(PORT_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(PORT_COMMAND, CMD_READ_SECTOR);
    ata_wait();

    for (int i = 0; i < 256; i++) {
        ((uint16_t*)buf)[i] = inw(PORT_DATA);
    }
}

void ata_write_sector(uint32_t lba, const uint8_t *buf) {
    ata_wait();
    outb(PORT_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(PORT_SECCOUNT, 1);
    outb(PORT_LBA_LOW,  lba & 0xFF);
    outb(PORT_LBA_MID,  (lba >> 8) & 0xFF);
    outb(PORT_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(PORT_COMMAND, CMD_WRITE_SECTOR);
    ata_wait();

    for (int i = 0; i < 256; i++) {
        outb(PORT_DATA, ((uint16_t*)buf)[i]);
    }
}

void load_file_table() {
    uint8_t buffer[SECTOR_SIZE];
    ata_read_sector(FILE_TABLE_SECTOR, buffer);
    for (int i = 0; i < MAX_FILES; i++) {
        file_table[i] = ((FileEntry*)buffer)[i];
    }
}

FileEntry* find_file(const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!memcmp(file_table[i].name, name, 16)) {
            return &file_table[i];
        }
    }
    return 0;
}

int read_file(const char *name, uint8_t *buffer) {
    FileEntry *file = find_file(name);
    if (!file || !(file->permissions & PERM_READ)) return -1;

    uint32_t sectors = (file->size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    for (uint32_t i = 0; i < sectors; i++) {
        ata_read_sector(file->start_sector + i, buffer + i * SECTOR_SIZE);
    }
    return file->size;
}

int write_file(const char *name, const uint8_t *buffer, uint32_t size) {
    FileEntry *file = find_file(name);
    if (!file || !(file->permissions & PERM_WRITE)) return -1;

    uint32_t sectors = (size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    for (uint32_t i = 0; i < sectors; i++) {
        ata_write_sector(file->start_sector + i, buffer + i * SECTOR_SIZE);
    }
    file->size = size;
    return 0;
}

