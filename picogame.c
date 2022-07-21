#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "vga.h"
#include "pico/platform.h"

#define FB_SIZE 320 * 240

uint16_t fb0[FB_SIZE];
uint16_t *p = &fb0[0];
//uint16_t fb1[FB_SIZE] = {0};

struct control_block {uint32_t len; uint16_t *data;};

struct control_block scanline_blocks[481];

void fill_scanline_blocks()
{
  for (int i = 0; i < 480; i++)
  {
    scanline_blocks[i].len = 320;
    scanline_blocks[i].data = &fb0[320 * (i/2)];
  }
  scanline_blocks[480].len = 0;
  scanline_blocks[480].data = NULL;
}

void frame_done_handler()
{
  dma_channel_set_read_addr(1, &scanline_blocks[0], true);
  dma_hw->ints0 = 1u;
}

int main()
{
  set_sys_clock_khz(250000, true);
  stdio_init_all();
  fill_scanline_blocks();

  smState vSyncState, hSyncState, rgbState;
  init_pio_vsync(&vSyncState);
  init_pio_hsync(&hSyncState);
  init_pio_rgb(&rgbState);

  uint16_t pixel_color = 0xFFDF;

  for (int i = 0; i < FB_SIZE; i++)
  {
    if (i / 320 == 0)
    {
      fb0[i] = 0x1f;
    }
    else
    {
      fb0[i] = 0x1f00;
    }
  }
  

  dma_channel_config config0 = dma_channel_get_default_config(0);
  channel_config_set_transfer_data_size(&config0, DMA_SIZE_16);
  channel_config_set_read_increment(&config0, true);
  channel_config_set_write_increment(&config0, false);
  channel_config_set_dreq(&config0, DREQ_PIO0_TX2);
  channel_config_set_chain_to(&config0, 1);
  channel_config_set_irq_quiet(&config0, true);

  dma_channel_configure(
      0,                       // Channel to be configured
      &config0,                // The configuration we just created
      &pio0->txf[rgbState.sm], // write address (RGB PIO TX FIFO)
      NULL,            // The initial read address (pixel color array)
      0,                 // Number of transfers; in this case each is one word.
      false                    // Don't start immediately.
  );
  dma_channel_set_irq0_enabled(0, true);
  irq_set_exclusive_handler(DMA_IRQ_0, frame_done_handler);
  irq_set_enabled(DMA_IRQ_0, true);

  // Channel One control channel
  dma_channel_config c1 = dma_channel_get_default_config(1); // default configs
  channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);   // 32-bit txfers
  channel_config_set_read_increment(&c1, true);            
  channel_config_set_write_increment(&c1, true);
  channel_config_set_ring(&c1, true, 3);            
  
  dma_channel_configure(
      1,                        // Channel to be configured
      &c1,                      // The configuration we just created
      &dma_hw->ch[0].al3_transfer_count, // Write address (channel 0 read address)
      &scanline_blocks[0],             // Read address (POINTER TO AN ADDRESS)
      2,                        // Number of transfers, in this case each is 1 word
      false                     // Don't start immediately.
  );

  pio_enable_sm_mask_in_sync(pio0, ((1u << hSyncState.sm) | (1u << vSyncState.sm) | (1u << rgbState.sm)));
  dma_start_channel_mask((1u << 1));

  while (true)
  {
    //pio_sm_put_blocking(rgbState.pio, rgbState.sm, 0xFFFFFFFF);
  }

  return 0;
}
