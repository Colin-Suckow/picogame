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

//uint32_t line_nums[480];

int main()
{
  set_sys_clock_khz(250000, true);
  stdio_init_all();

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

  dma_channel_configure(
      0,                       // Channel to be configured
      &config0,                // The configuration we just created
      &pio0->txf[rgbState.sm], // write address (RGB PIO TX FIFO)
      &fb0,            // The initial read address (pixel color array)
      FB_SIZE,                 // Number of transfers; in this case each is one word.
      false                    // Don't start immediately.
  );

  // Channel One (reconfigures the first channel)
  dma_channel_config c1 = dma_channel_get_default_config(1); // default configs
  channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);   // 32-bit txfers
  channel_config_set_read_increment(&c1, false);             // no read incrementing
  channel_config_set_write_increment(&c1, false);            // no write incrementing
  channel_config_set_chain_to(&c1, 0);                       // chain to other channel

  //uint16_t *p = &pixel_color;
  uint16_t **pp = &p;
  uint16_t ***ppp = &pp;
  
  dma_channel_configure(
      1,                        // Channel to be configured
      &c1,                      // The configuration we just created
      &dma_hw->ch[0].read_addr, // Write address (channel 0 read address)
      &p,             // Read address (POINTER TO AN ADDRESS)
      1,                        // Number of transfers, in this case each is 1 word
      false                     // Don't start immediately.
  );

  pio_enable_sm_mask_in_sync(pio0, ((1u << hSyncState.sm) | (1u << vSyncState.sm) | (1u << rgbState.sm)));
  dma_start_channel_mask((1u << 0));

  while (true)
  {
    //pio_sm_put_blocking(rgbState.pio, rgbState.sm, 0xFFFFFFFF);
    //printf("fb0[0]: %x, fb0: %x, &fb0: %x, &fb0[0]: %x\n", fb0[0], fb0, &fb0, &fb0[0]);
  }

  return 0;
}
