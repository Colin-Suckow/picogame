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

int main()
{
  set_sys_clock_khz(250000, true);
  stdio_init_all();

  vga_init();

  while (true)
  {
    time += 0.1;
    vga_clear();
    draw_box(FB_WIDTH / 2 + ( sinf(time) * 50) - 25, FB_HEIGHT / 2 + (cosf(time) * 50) - 25, 50, 50);
    sleep_ms(17);
  }

  return 0;
}
