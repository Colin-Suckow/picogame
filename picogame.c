#include "3d.h"
#include "vga.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"


void main_core1()
{
  vga_init();

  while (true)
  {}
}

int main()
{
  set_sys_clock_khz(250000, true);
  stdio_init_all();

  multicore_launch_core1(main_core1);

  absolute_time_t t0, t1;
  int framerate = 0;

  float time = 0.0;
  while (true)
  {
    t0 = get_absolute_time();
    
    time += 0.001;
    if (time > M_PI * 2) time = 0;
    
    vga_clear();
    vga_draw_str(0, 0,"FPS: %d", framerate);
    //vga_draw_triangle(vec2_new(20, 20), vec2_new(116, 60), vec2_new(40, 115), 0xFF);
    vga_draw_line(vec2_new(FB_WIDTH / 2, FB_HEIGHT / 2), vec2_new(sinf(time) * 50 + (FB_WIDTH / 2), cosf(time) * 50 + (FB_HEIGHT / 2)), vga_create_color(128, 128, 255));
    vga_swap_buffer();
    t1 = get_absolute_time();
    framerate =  (int) (1.0 / ((float) absolute_time_diff_us(t0, t1) / 1000000.0));
  }

  return 0;
}