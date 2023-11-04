#include <syscall.h>

extern int main(int argc, char **argv);

int entry(int argc, char **argv) {
    main(argc, argv);

    // free memory
    c_mem_free_all(c_process_get_pid());

    return 0;
}
