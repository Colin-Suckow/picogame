#include "3d.h"

Vec vec2_new(int x, int y)
{
  Vec result;
  result.x = x;
  result.y = y;
  result.z = 0;
  result.w = 0;
  return result;
}