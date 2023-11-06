#include <sys/stat.h>
#include <syscall.h>
#include <filesys.h>
#include <profan.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>

// __udivdi3 definition
unsigned long long __udivdi3(unsigned long long num, unsigned long long den) {
    unsigned long long quot = 0, qbit = 1;

    if (den == 0) {
        return 0; // divide by zero
    }

    // Left-justify denominator and count shift
    while ((signed long long)den >= 0) {
        den <<= 1;
        qbit <<= 1;
    }

    while (qbit) {
        if (den <= num) {
            num -= den;
            quot += qbit;
        }
        den >>= 1;
        qbit >>= 1;
    }

    return quot;
}

// __divdi3 definition
signed long long __divdi3(signed long long num, signed long long den) {
    int neg = 0;

    if (num < 0) {
        num = -num;
        neg = !neg;
    }
    if (den < 0) {
        den = -den;
        neg = !neg;
    }

    return neg ? -__udivdi3(num, den) : __udivdi3(num, den);
}

// __umoddi3 definition
unsigned long long __umoddi3(unsigned long long num, unsigned long long den) {
    if (den == 0) {
        return num; // divide by zero
    }

    // Left-justify denominator and count shift
    while ((signed long long)den >= 0) {
        den <<= 1;
    }

    while (den > num) {
        den >>= 1;
    }

    return num - den;
}


// __moddi3 definition
long long __moddi3(long long a, long long b) {
    return a - __divdi3(a, b) * b;
}

typedef struct {
    int offset;
    sid_t sid;
} local_file_t;

local_file_t **open_files;

void init_bordel(void) {
    open_files = malloc(sizeof(local_file_t *) * 32);
    for (int i = 0; i < 32; i++) {
        open_files[i] = NULL;
    }
}

int open(const char *pathname, int flags, ...) {
    char *pwd = getenv("PWD");
    if (pwd == NULL) pwd = "/";

    char *full_path = malloc(strlen(pwd) + strlen(pathname) + 2);
    assemble_path(pwd, (char *) pathname, full_path);

    printf("open called with path: %s\n", full_path);
    sid_t sid = fu_path_to_sid(ROOT_SID, (char *) full_path);
    free(full_path);

    if (IS_NULL_SID(sid)) {
        return -1;
    }

    for (int i = 0; i < 32; i++) {
        if (open_files[i] == NULL) {
            open_files[i] = malloc(sizeof(local_file_t));
            open_files[i]->offset = 0;
            open_files[i]->sid = sid;
            return i;
        }
    }
    return -1;
}

ssize_t read(int fd, void *buf, size_t count) {
    if (fd < 0 || fd >= 32 || open_files[fd] == NULL) {
        return -1;
    }

    sid_t sid = open_files[fd]->sid;
    uint32_t filesize = fu_get_file_size(sid);
    uint32_t offset = open_files[fd]->offset;
    
    if (offset >= filesize) {
        return 0;
    }

    uint32_t read_size = count;
    
    if (offset + count > filesize) {
        read_size = filesize - offset;
    }
    
    fu_file_read(sid, buf, offset, read_size);
    open_files[fd]->offset += read_size;
    return read_size;
}

ssize_t write(int fd, const void *buf, size_t count) {
    if (fd < 0 || fd >= 32 || open_files[fd] == NULL) {
        return -1;
    }

    sid_t sid = open_files[fd]->sid;
    uint32_t filesize = fu_get_file_size(sid);
    uint32_t offset = open_files[fd]->offset;

    if (offset + count > filesize) {
        fu_set_file_size(sid, offset + count);
    }

    fu_file_write(sid, (void *) buf, offset, count);
    open_files[fd]->offset += count;
    return count;
}

int close(int fd) {
    if (fd < 0 || fd >= 32 || open_files[fd] == NULL) {
        return -1;
    }

    printf("close called\n");
    free(open_files[fd]);
    open_files[fd] = NULL;
    return 0;
}

off_t lseek(int fd, off_t offset, int whence) {
    if (fd < 0 || fd >= 32 || open_files[fd] == NULL) {
        return -1;
    }
    if (whence == SEEK_SET) {
        open_files[fd]->offset = offset;
    } else if (whence == SEEK_CUR) {
        open_files[fd]->offset += offset;
    } else if (whence == SEEK_END) {
        sid_t sid = open_files[fd]->sid;
        uint32_t filesize = fu_get_file_size(sid);
        open_files[fd]->offset = filesize + offset;
    }
    return open_files[fd]->offset;
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
