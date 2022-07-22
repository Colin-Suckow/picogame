#include "vga.h"
#include "font.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "pico/platform.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "hsync.pio.h"
#include "vsync.pio.h"
#include "rgb.pio.h"

#define DMA_DATA_CHANNEL 0
#define DMA_CONTROL_CHANNEL 1

uint16_t framebuffer[FB_SIZE] = {0};


typedef struct
{
  PIO pio;
  uint sm;
} smState;

struct control_block {uint32_t len; uint16_t *data;};

struct control_block scanline_blocks[481];

void (*queued_draw_function)();

// PRIVATE

void fill_scanline_blocks()
{
  for (int i = 0; i < 480; i++)
  {
    scanline_blocks[i].len = 320;
    scanline_blocks[i].data = &framebuffer[320 * (i/2)];
  }
  scanline_blocks[480].len = 0;
  scanline_blocks[480].data = NULL;
}

void frame_done_handler()
{
  dma_channel_set_read_addr(1, &scanline_blocks[0], true);
  dma_hw->ints0 = 1u;
}

void vsync_handler()
{
  if (pio0_hw->irq & 2)
  {
    if (queued_draw_function != NULL)
    {
      queued_draw_function();
      queued_draw_function = NULL;
    }
    hw_set_bits(&pio0->irq, 2u);
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

  //Set up irq
  irq_set_exclusive_handler(PIO0_IRQ_0, vsync_handler);
  irq_set_enabled(PIO0_IRQ_0, true);
  pio0_hw->inte0 = PIO_IRQ0_INTE_SM1_BITS;

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
  channel_config_set_chain_to(&data_config, DMA_CONTROL_CHANNEL);
  channel_config_set_irq_quiet(&data_config, true);

  dma_channel_configure(
      0,
      &data_config,
      &pio0->txf[rgbState->sm],
      NULL,            // Read addr and len will be set by the control blocks
      0,
      false
  );
  dma_channel_set_irq0_enabled(DMA_DATA_CHANNEL, true);
  irq_set_exclusive_handler(DMA_IRQ_0, frame_done_handler);
  irq_set_enabled(DMA_IRQ_0, true);

  // Channel One, Control channel
  dma_channel_config control_config = dma_channel_get_default_config(DMA_CONTROL_CHANNEL);
  channel_config_set_transfer_data_size(&control_config, DMA_SIZE_32);
  channel_config_set_read_increment(&control_config, true);            
  channel_config_set_write_increment(&control_config, true);
  channel_config_set_ring(&control_config, true, 3);            
  
  dma_channel_configure(
      DMA_CONTROL_CHANNEL,
      &control_config,
      &dma_hw->ch[DMA_DATA_CHANNEL].al3_transfer_count, // Write to the data DMA channel registers
      &scanline_blocks[0],                              // Start reading from the command block array
      2,
      false
  );
}

void draw_character(int x, int y, char *char_raster)
{
  for (int line = 0; line < FONT_CHAR_HEIGHT; line++)
  {
    for (int pixel = 0; pixel < FONT_CHAR_WIDTH; pixel++)
    {
      if ((char_raster[line] >> (FONT_CHAR_WIDTH - pixel)) & 1 == 1)
      {
        framebuffer[(y + (FONT_CHAR_HEIGHT - line)) * FB_WIDTH + (x + pixel)] = 0xFFFF;
      }
    }
  }
}

// public

void vga_draw_str(int x, int y, const char *text, ...)
{
  int char_offset = 0;
  char textBuf[64];
  va_list arg;
  va_start(arg, text);

  vsprintf(textBuf, text, arg);
  va_end(arg);

  for (int i = 0; i < strlen(textBuf); i++)
  {
    if (text[i] >= 32 && text[i] <= 126)
    {
      draw_character(x + char_offset, y, (char *) &font_rasters[textBuf[i] - 32]);
      char_offset += FONT_CHAR_WIDTH;
    }
  }
}

void vga_init()
{
  smState vSyncState, hSyncState, rgbState;
  
  fill_scanline_blocks();

  init_pio_vsync(&vSyncState);
  init_pio_hsync(&hSyncState);
  init_pio_rgb(&rgbState);

  configure_dma(&rgbState);

  pio_enable_sm_mask_in_sync(pio0, ((1u << hSyncState.sm) | (1u << vSyncState.sm) | (1u << rgbState.sm)));
  dma_start_channel_mask((1u << 1));
}

void vga_clear()
{
  for (int i = 0; i < FB_SIZE; i++)
  {
    framebuffer[i] = 0;
  }
}

uint16_t vga_create_color(uint8_t red, uint8_t green, uint8_t blue)
{
  uint16_t result = 0;
  result |= (red >> 3);
  result |= (green >> 3) << 7;
  result |= (blue >> 3) << 12;
}

void vga_queue_draw(void (*draw_function)())
{
  queued_draw_function = draw_function;
}