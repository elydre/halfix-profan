#ifndef FCNTL_H
#define FCNTL_H

#define O_BINARY 0x8000
#define O_RDONLY 0x0000
#define O_RDWR 0x0002
#define O_CREAT 0x0100
#define O_TRUNC 0x0200
#define O_WRONLY 0x0001

int open(const char *pathname, int flags, ...);

#endif
