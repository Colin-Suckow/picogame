## Goals
320x240, 15 bit color, double buffered display controller
Handled entirely by PIO, no cpu intervention required (other than load framebuffer)

## Progress

# Generating the vga signal
Basically a copy of https://vanhunteradams.com/Pico/VGA/VGA.html

Changed to accommodate 15 bit color and lower resolution

### Current issues
- horizontal resolution is correct, but vertical image is doubled since I need to send each scaneline twice. Doing so will require some different DMA shenanigans

# Framebuffer
Simple array of uint16_t's. The main problem is ram. Theres only 264KB of ram and each framebuffer takes ~153KB. Double buffering requires more memory than the pico has. There is 2MB of flash, but store the buffer there is not viable due to flash wear. At 60fps the write load would surpass the rated number of writes in under half an hour.

## Options
- External SRAM chip
  - Pro: FUll color with full 640 x 480 framebuffers possible
  - Con: Involves buying and connecting new hardware. Unsure if SPI bus is fast enough
- Reduce color resolution
  - Pro: Keep double buffering
  - Con: Reduced color resolution and difficult to implement because of the PIO sequential GPIO requirement
- Drop double buffer requirement
  - Con: Screen tearing
  - Pro: Easy
- Implement wear leveling on flash
  - Rough math shows ~6 hours longevity
- Reduce display resolution to 160 x 120
  - It would work, but this is getting pretty low res

Right now I have decided to drop the double buffering requirement. Maybe later I can try using SRAM.