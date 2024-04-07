#ifndef SYS_STAT_H
#define SYS_STAT_H

#include <profan/type.h>

#define dev_t unsigned int
#define ino_t unsigned int
#define mode_t unsigned int
#define nlink_t unsigned int
// #define uid_t unsigned int
// #define gid_t unsigned int
// #define off_t unsigned int
#define blksize_t unsigned int
#define blkcnt_t unsigned int
// #define time_t unsigned int

struct stat {
    dev_t     st_dev;      /* ID du périphérique contenant le fichier */
    ino_t     st_ino;      /* Numéro inœud */
    mode_t    st_mode;     /* Protection */
    nlink_t   st_nlink;    /* Nb liens matériels */
    uid_t     st_uid;      /* UID propriétaire */
    gid_t     st_gid;      /* GID propriétaire */
    dev_t     st_rdev;     /* ID périphérique (si fichier spécial) */
    off_t     st_size;     /* Taille totale en octets */
    blksize_t st_blksize;  /* Taille de bloc pour E/S */
    blkcnt_t  st_blocks;   /* Nombre de blocs alloués */
    time_t    st_atime;    /* Heure dernier accès */
    time_t    st_mtime;    /* Heure dernière modification */
    time_t    st_ctime;    /* Heure dernier changement état */
};

int fstat(int fd, struct stat *buf);
int mkdir(const char *pathname, mode_t mode);

// S_ISDIR
#define S_ISDIR(mode) 0

#endif
