#include <sys/stat.h>
#include <stdio.h>

int fstat(int fd, struct stat *buf) {
    printf("fstat called\n");
    return 0;
}

int mkdir(const char *pathname, mode_t mode) {
    printf("mkdir called\n");
    return 0;
}
