#ifndef THREE_D_H
#define THREE_D_H

#include "pico/stdlib.h"

typedef struct {
  int x;
  int y;
  int z;
  int w;
} Vec;

Vec vec_new(int x, int y, int z, int w);
Vec vec2_new(int x, int y);

typedef struct {
  Vec *p1;
  Vec *p2;
  Vec *p3;
  uint16_t color;
} ModelFace;



#endif