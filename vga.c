#include "vga.h"
#include "font.h"
#include "3d.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "pico/platform.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "hsync.pio.h"
#include "vsync.pio.h"
#include "rgb.pio.h"

#define DMA_DATA_CHANNEL 0

#define FB_SIZE FB_WIDTH * FB_HEIGHT

#define WORKING_FB_OFFSET (!active_framebuffer * FB_SIZE)
#define DRAWING_FB_OFFSET (active_framebuffer * FB_SIZE)

uint8_t framebuffer[FB_SIZE * 2] = {0}; // Store both framebuffers in one array
uint8_t active_framebuffer = 0;

typedef struct
{
  PIO pio;
  uint sm;
} smState;

// PRIVATE

void scanline_handler()
{
  static int current_scanline = 0;
  static uint16_t active_scanline[FB_WIDTH] = {0};
  static uint16_t next_scanline[FB_WIDTH] = {0};

  // Copy next scanline to current
  for (int i = 0; i < FB_WIDTH; i++)
  {
    active_scanline[i] = next_scanline[i];
  }

  dma_channel_set_read_addr(DMA_DATA_CHANNEL, &active_scanline[0], false);
  dma_channel_set_trans_count(DMA_DATA_CHANNEL, FB_WIDTH, true);
  dma_hw->ints0 = 1u;
  if (++current_scanline >= 480) current_scanline = 0;

  // Unpack next scanline

  int scanline_start = (current_scanline / 2) * FB_WIDTH;

  for (int i = 0; i < FB_WIDTH; i++)
  {
    uint8_t pixel = framebuffer[DRAWING_FB_OFFSET + scanline_start + i];
    uint16_t unpacked_pixel = 0;

    unpacked_pixel |= (pixel & 0x7) << 2; // red
    unpacked_pixel |= (pixel & 0x38) << 5; // green
    unpacked_pixel |= (pixel & 0xC0) << 8; // blue

    next_scanline[i] = unpacked_pixel;
  }
}

void init_pio_hsync(smState *state)
{
  PIO pio = pio0;
  uint offset = pio_add_program(pio, &hsync_program);
  uint sm = 0;
  hsync_program_init(pio, sm, offset, 16);
  pio_sm_put_blocking(pio, sm, 655); // Visible + front porch
  state->pio = pio;
  state->sm = sm;
}

void init_pio_vsync(smState *state)
{
  PIO pio = pio0;
  uint offset = pio_add_program(pio, &vsync_program);
  uint sm = 1;
  vsync_program_init(pio, sm, offset, 17);
  pio_sm_put_blocking(pio, sm, 479); // Visible

  state->pio = pio;
  state->sm = sm;
}

void init_pio_rgb(smState *state)
{
  PIO pio = pio0;
  uint offset = pio_add_program(pio, &rgb_program);
  uint sm = 2;
  rgb_program_init(pio, sm, offset);
  pio_sm_put_blocking(pio, sm, 319); // pixels to pull for each scanline
  state->pio = pio;
  state->sm = sm;
}

void configure_dma(smState *rgbState)
{
  dma_channel_config data_config = dma_channel_get_default_config(DMA_DATA_CHANNEL);
  channel_config_set_transfer_data_size(&data_config, DMA_SIZE_16);
  channel_config_set_read_increment(&data_config, true);
  channel_config_set_write_increment(&data_config, false);
  channel_config_set_dreq(&data_config, DREQ_PIO0_TX2);

  dma_channel_configure(
      0,
      &data_config,
      &pio0->txf[rgbState->sm],
      NULL,            // Read addr and len will be set by the scanline handler
      0,
      false
  );
  dma_channel_set_irq0_enabled(DMA_DATA_CHANNEL, true);
  irq_set_exclusive_handler(DMA_IRQ_0, scanline_handler);
  irq_set_enabled(DMA_IRQ_0, true);
}

void draw_character(int x, int y, char *char_raster)
{
  for (int line = 0; line < FONT_CHAR_HEIGHT; line++)
  {
    for (int pixel = 0; pixel < FONT_CHAR_WIDTH; pixel++)
    {
      if ((char_raster[line] >> (pixel)) & 1 == 1)
      {
        framebuffer[WORKING_FB_OFFSET + (y + line) * FB_WIDTH + (x + pixel)] = 0xFF;
      }
    }
  }
}

int max(int a, int b)
{
  if (a > b) return a;
  else return b;
}

int min(int a, int b)
{
  if (a > b) return b;
  else return a;
}

// public

void vga_draw_str(int x, int y, const char *text, ...)
{
  int char_offset = 0;
  int y_offset = 0;
  char textBuf[256];
  va_list arg;
  va_start(arg, text);

  vsprintf(textBuf, text, arg);
  va_end(arg);

  for (int i = 0; i < strlen(textBuf); i++)
  {
    if (text[i] == 0xA) // Handle newline character
    {
      char_offset = 0;
      y_offset += FONT_CHAR_HEIGHT;
    }
    else if (text[i] >= 0 && text[i] <= 128)
    {
      draw_character(x + char_offset, y + y_offset, (char *) &font_rasters[textBuf[i]]);
      char_offset += FONT_CHAR_WIDTH;
    }
  }
}

void vga_init()
{
  smState vSyncState, hSyncState, rgbState;
  
  init_pio_vsync(&vSyncState);
  init_pio_hsync(&hSyncState);
  init_pio_rgb(&rgbState);

  configure_dma(&rgbState);

  pio_enable_sm_mask_in_sync(pio0, ((1u << hSyncState.sm) | (1u << vSyncState.sm) | (1u << rgbState.sm)));
  dma_start_channel_mask((1u << DMA_DATA_CHANNEL));
}

void vga_swap_buffer()
{
  active_framebuffer = !active_framebuffer;
}

void vga_clear()
{
  for (int i = 0; i < FB_SIZE; i++)
  {
    framebuffer[WORKING_FB_OFFSET + i] = 0;
  }
}

uint8_t vga_create_color(uint8_t red, uint8_t green, uint8_t blue)
{
  uint8_t result = 0;
  result |= (red >> 5);
  result |= (green >> 5) << 3;
  result |= (blue >> 6) << 6;
  return result;
}

void vga_queue_draw(void (*draw_function)())
{
}

void vga_draw_line(Vec p1, Vec p2, uint8_t color)
{
  if ((int) p1.y == (int) p2.y)
  {
    int y = (int) p1.y;
    int min = (int) fmin(p1.x, p2.x);
    int max = (int) fmax(p1.x, p2.x);
    for (int i = min; i <= max; i++)
    {
      framebuffer[y * FB_WIDTH + i + WORKING_FB_OFFSET] = color;
    }
  }
  else if ((int) p1.x == (int) p2.x)
  {
    int x = (int) p1.x;
    int min = (int) fmin(p1.y, p2.y);
    int max = (int) fmax(p1.y, p2.y);
    for (int i = min; i <= max; i++)
    {
      framebuffer[i * FB_WIDTH + x + WORKING_FB_OFFSET] = color;
    }
  }
  else
  {
    // http://members.chello.at/easyfilter/bresenham.html
    int x0 = (int) p1.x, y0 = (int) p1.y;
    int x1 = (int) p2.x, y1 = (int) p2.y;
    int dx = abs((int) (x1 - x0)), sx = x0 < x1 ? 1 : -1;
    int dy = -abs((int) (y1 - y0)), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;)
    {
      framebuffer[y0 * FB_WIDTH + x0 + WORKING_FB_OFFSET] = color;
      if (x0 == x1 && y0 == y1) break;
      e2 = 2 * err;
      if (e2 >= dy) {err += dy; x0 += sx;}
      if (e2 <= dx) {err += dx; y0 += sy;}
    }
  }
}

void vga_draw_triangle(Vec p1, Vec p2, Vec p3, uint8_t color)
{
  int max_x = max(p1.x, max(p2.x, p3.x));
  int max_y = max(p1.y, max(p2.y, p3.y));
  int min_x = min(p1.x, min(p2.x, p3.x));
  int min_y = min(p1.y, min(p2.y, p3.y));
  int bb_width = max_x - min_x;
  int bb_height = max_y - min_y;

  int A01 = p1.y - p2.y, B01 = p2.x - p1.x;
  int A12 = p2.y - p3.y, B12 = p3.x - p2.x;
  int A20 = p3.y - p1.y, B20 = p1.x - p3.x;

  int edge_function(const Vec *a, const Vec *b, const Vec *c)
  {
    return (b->x-a->x)*(c->y-a->y) - (b->y-a->y)*(c->x-a->x);
  }

  Vec p = vec2_new(min_x, min_y);

  int w0_row = edge_function(&p2, &p3, &p);
  int w1_row = edge_function(&p3, &p1, &p);
  int w2_row = edge_function(&p1, &p2, &p);

  for (p.y = min_y; p.y <= max_y; p.y++)
  {
    int w0 = w0_row;
    int w1 = w1_row;
    int w2 = w2_row;
    for (p.x = min_x; p.x <= max_x; p.x++)
    {
      if (w0 >= 0 && w1 >= 0 && w2 >= 0)
      {
        framebuffer[p.y * FB_WIDTH + p.x + WORKING_FB_OFFSET] = color;
      }

      w0 += A12;
      w1 += A20;
      w2 += A01;
    }
    w0_row += B12;
    w1_row += B20;
    w2_row += B01;
  }
}