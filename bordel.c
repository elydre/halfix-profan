#include <sys/stat.h>
#include <stdio.h>
#include <zlib.h>

// __umoddi3 definition
unsigned long long __umoddi3(unsigned long long a, unsigned long long b) {
    return a % b;
}

// __udivdi3 definition
unsigned long long __udivdi3(unsigned long long a, unsigned long long b) {
    return a / b;
}

// __divdi3 definition
long long __divdi3(long long a, long long b) {
    return a / b;
}

// __moddi3 definition
long long __moddi3(long long a, long long b) {
    return a % b;
}

ssize_t read(int fd, void *buf, size_t count) {
    printf("read called\n");
    return 0;
}

ssize_t write(int fd, const void *buf, size_t count) {
    printf("write called\n");
    return 0;
}

int open(const char *pathname, int flags, ...) {
    printf("open called\n");
    return 0;
}

int inflateInit_ (z_streamp strm, const char *version, int stream_size) {
    printf("inflateInit_ called\n");
    return 0;
}

int inflate (z_streamp strm, int flush) {
    printf("inflate called\n");
    return 0;
}

int inflateEnd (z_streamp strm) {
    printf("inflateEnd called\n");
    return 0;
}

int fstat(int fd, struct stat *buf) {
    printf("fstat called\n");
    return 0;
}

int mkdir(const char *pathname, mode_t mode) {
    printf("mkdir called\n");
    return 0;
}
