#ifndef GENERATOR_H
#define GENERATOR_H

enum {
  TILE_NONE = 0,

  // stone walls
  TILE_STONE_HWALL,
  TILE_STONE_VWALL,

  TILE_WOOD_FLOOR,
  TILE_WOOD_HOLE,
  TILE_STONE_FLOOR,

  TILE_DOOR_CLOSED,
  TILE_DOOR_OPEN,

  TILE_EXIT,

  TILE_NUM
};

#define SOLID_COUNT 4
static char solid[SOLID_COUNT] = {
  TILE_NONE,

  TILE_STONE_HWALL,
  TILE_STONE_VWALL,

  TILE_WOOD_HOLE
};

static int check_solid(int tile) {
  for (int i=0; i<SOLID_COUNT; i++)
    if (tile == solid[i])
      return 1;

  return 0;
}

int gen(map_t *map);

#endif // GENERATOR_H