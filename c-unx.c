#ifndef C_UNIX_C
#define C_UNIX_C

#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#define PROT_READ     0x1
#define PROT_WRITE    0x2
#define PROT_EXEC     0x4

#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED    ((void *) -1)

static void testFile() {
    printf("MMAP For Unix/Linux\n");
}

static void* mmap_file(const char* filename, size_t* length) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return MAP_FAILED;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        perror("Error getting file size");
        close(fd);
        return MAP_FAILED;
    }

    void* addr = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        perror("Error mapping file to memory");
        close(fd);
        return MAP_FAILED;
    }

    *length = (size_t)file_size;

    close(fd);
    return addr;
}

static void munmap_file(void* addr, size_t length) {
    munmap(addr, length);
}

#endif