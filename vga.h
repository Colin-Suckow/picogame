#ifndef VGA_H
#define VGA_H

#include "pico/stdlib.h"

#define FB_WIDTH 320
#define FB_HEIGHT 240
#define FB_SIZE FB_WIDTH * FB_HEIGHT


extern uint16_t framebuffer[];

void vga_init();
void vga_set_enable(bool enabled);
void vga_clear();
uint16_t vga_create_color(uint8_t red, uint8_t green, uint8_t blue);

#endif