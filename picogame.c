#include "3d.h"
#include "vga.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"

void draw_box(int x, int y, int width, int height)
{

  for (int box_y = 0; box_y < height; box_y++)
  {
    for (int box_x = 0; box_x < width; box_x++)
    {
      framebuffer[(box_y + y) * FB_WIDTH + (box_x + x)] = 0x1f;
    }
  }
}

float time = 0.0;

void draw_func()
{
  time += 0.1;
  if (time > M_PI * 2) time = 0;
  vga_clear();
  vga_draw_str(0, 0,"Hello World!");
  vga_draw_line(vec2_new(FB_WIDTH / 2, FB_HEIGHT / 2), vec2_new(sinf(time) * 50 + (FB_WIDTH / 2), cosf(time) * 50 + (FB_HEIGHT / 2)), vga_create_color(128, 128, 255));
  vga_draw_triangle(vec2_new(20, 20), vec2_new(40, 15),  vec2_new(16, 60), 0xFFFF);
}

int main()
{
  set_sys_clock_khz(250000, true);
  stdio_init_all();

  vga_init();

  while (true)
  {
    vga_queue_draw(draw_func);
  }

  return 0;
}
