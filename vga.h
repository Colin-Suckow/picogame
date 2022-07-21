#ifndef VGA_H
#define VGA_H

#include "pico/stdlib.h"

#define FB_WIDTH 320
#define FB_HEIGHT 240
#define FB_SIZE FB_WIDTH * FB_HEIGHT


extern uint16_t framebuffer[];

void vga_init();
void vga_set_enable(bool enabled);

#endif