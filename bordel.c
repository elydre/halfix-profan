#include <sys/stat.h>
#include <stdio.h>

unsigned long long __udivdi3(unsigned long long num, unsigned long long den) {
    unsigned long long quotient = 0;
    unsigned long long remainder = 0;
    unsigned long long mask = 1ULL << 63; // Le bit le plus significatif

    for(int i = 0; i < 64; i++) {
        remainder = (remainder << 1) | ((num & mask) >> 63); // Met à jour le reste
        num = num << 1; // Décalage à gauche de num
        quotient = quotient << 1; // Décalage à gauche du quotient

        if (remainder >= den) {
            remainder -= den;
            quotient |= 1; // Met à jour le bit de quotient
        }
    }

    return quotient;
}

signed long long __divdi3(signed long long num, signed long long den) {
    int sign = 1;
    if ((num < 0 && den > 0) || (num > 0 && den < 0)) {
        sign = -1;
    }

    unsigned long long unsigned_num = (num < 0) ? -num : num;
    unsigned long long unsigned_den = (den < 0) ? -den : den;

    unsigned long long quotient = __udivdi3(unsigned_num, unsigned_den);

    return sign * quotient;
}

unsigned long long __umoddi3(unsigned long long num, unsigned long long den) {
    unsigned long long quotient = __udivdi3(num, den);
    unsigned long long remainder = num - den * quotient;
    return remainder;
}

long long __moddi3(long long a, long long b) {
    int sign = 1;
    if (a < 0) {
        sign = -1;
    }

    unsigned long long unsigned_a = (a < 0) ? -a : a;
    unsigned long long unsigned_b = (b < 0) ? -b : b;

    unsigned long long remainder = __umoddi3(unsigned_a, unsigned_b);

    return sign * remainder;
}

int fstat(int fd, struct stat *buf) {
    printf("fstat called\n");
    return 0;
}

int mkdir(const char *pathname, mode_t mode) {
    printf("mkdir called\n");
    return 0;
}

// __udivmoddi4 definition
unsigned long long __udivmoddi4(unsigned long long num, unsigned long long den, unsigned long long *rem_p) {
    unsigned long long quot = 0, qbit = 1;
    unsigned long long rem = 0;

    if (den == 0) {
        return 0; // divide by zero
    }

    // Left-justify denominator and count shift
    while ((signed long long) den >= 0) {
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

    rem = num;

    *rem_p = rem;
    return quot;
}

// __divmoddi4 definition
long long __divmoddi4(long long num, long long den, long long *rem_p) {
    int sign = 1;
    if ((num < 0 && den > 0) || (num > 0 && den < 0)) {
        sign = -1;
    }

    unsigned long long unsigned_num = (num < 0) ? -num : num;
    unsigned long long unsigned_den = (den < 0) ? -den : den;

    unsigned long long remainder = 0;
    unsigned long long quotient = __udivmoddi4(unsigned_num, unsigned_den, &remainder);

    *rem_p = sign * remainder;
    return sign * quotient;
}
