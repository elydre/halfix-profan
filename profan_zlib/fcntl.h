#ifndef FCNTL_H
#define FCNTL_H

#include <profan.h>

#define O_BINARY    0

#define open(path, flags, ...) profan_open(path, flags, ##__VA_ARGS__)

#endif
