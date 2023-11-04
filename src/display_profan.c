#ifdef PROFAN

#include <syscall.h>
#include <stdlib.h>

// display_update
// display_handle_events

int x, y;
uint32_t *screen;

void display_init(void) {
    screen = NULL;
    x = y = 0;
}

void *display_get_pixels(void) {
    return screen;
}

void display_set_resolution(int width, int height) {
    if (screen)
        free(screen);
    screen = malloc(width * height * sizeof(uint32_t));
    x = width;
    y = height;
}

void display_sleep(int ms) {
    c_process_sleep(ms, c_process_get_pid());
}

void display_release_mouse(void) {
    ;
}

void display_handle_events(void) {
    ;
}

void display_update(void) {
    for (int i = 0; i < x; i++)
        for (int j = 0; j < y; j++)
            c_vesa_set_pixel(i, j, screen[i + j * x]);
}


#endif // PROFANOS
