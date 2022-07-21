#ifndef VGA_H
#define VGA_H

#include "hardware/pio.h"

#include "hsync.pio.h"
#include "vsync.pio.h"
#include "rgb.pio.h"

typedef struct
{
  PIO pio;
  uint sm;
} smState;

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
  pio_sm_put_blocking(pio, sm, 319); // pixels to pull
  state->pio = pio;
  state->sm = sm;
}

#endif