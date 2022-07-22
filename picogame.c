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

  int box_x = FB_WIDTH / 2 + ( sinf(time) * 50) - 25;
  int box_y = FB_HEIGHT / 2 + (cosf(time) * 50) - 25;

  vga_clear();
  draw_box(box_x, box_y, 50, 50);
  vga_draw_str(0,0,"x: %d", box_x);
  vga_draw_str(0,13,"y: %d", box_y);
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
