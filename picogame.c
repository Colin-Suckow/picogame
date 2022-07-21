#include "vga.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"



int main()
{
  set_sys_clock_khz(250000, true);
  stdio_init_all();

  vga_init();

  for (int i = 0; i < FB_SIZE; i++)
  {
    int x = i % FB_WIDTH;
    int y = i / FB_WIDTH;
    if (x == 0 || y == 0 || x == FB_WIDTH - 1 || y == FB_HEIGHT - 1)
    {
      framebuffer[i] = 0x1f;
    }
  }
  
  while (true)
  {
  }

  return 0;
}
