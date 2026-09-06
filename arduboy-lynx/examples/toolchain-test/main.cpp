#include "Lynx.h"
#include <string.h>

#define BUFSIZE 8160
uint8_t fb[2][BUFSIZE] __attribute__((aligned(256)));
static uint8_t fbIndex = 0;

void draw_rect(uint8_t *buf, int x, int y, int w, int h, uint8_t color) {
    for (int r = y; r < y + h; r++) {
        uint8_t *line = buf + (r * 80) + (x / 2);
        // Each byte is two pixels. Use the same color for both.
        uint8_t val = (color << 4) | color;
        for (int c = 0; c < w / 2; c++) {
            line[c] = val;
        }
    }
}

int main() {
    lynx_clock_init();
    lynx_video_init(fb[0]);

    uint8_t frameCounter = 0;

    while (1) {
        uint8_t *back = fb[fbIndex ^ 1];
        memset(back, 0, BUFSIZE);

        // This rectangle will change color based on frameCounter
        // If lynx_wait_for_frame is working, this will animate.
        // If it's timing out or hanging, it will stay one color or be black.
        draw_rect(back, 10, 10, 40, 20, frameCounter % 16);
        
        draw_rect(back, 60, 10, 40, 20, 0x2); // Red
        draw_rect(back, 110, 10, 40, 20, 0x4); // Green
        
        lynx_swap_display(back);
        fbIndex ^= 1;
        frameCounter++;
    }
    return 0;
}
