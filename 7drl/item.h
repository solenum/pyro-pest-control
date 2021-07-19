#ifndef ITEM_H
#define ITEM_H

#include "math.h"
#include <inttypes.h>

enum {
  ITEM_NONE,

  ITEM_FIRETORCH,

  ITEM_FIREBOLT,
  ITEM_FIRESURGE,
  ITEM_FIRESTORM,
  ITEM_FIRESPRAY,
  ITEM_FIREPUSH,
  ITEM_FIREJUMP,
  ITEM_SPIRIT,
  ITEM_FOOD,
  ITEM_FLESH,
  ITEM_POT,

  ITEM_NUM
};

enum {
  ITEM_TILE_FIRETORCH = 36,
  ITEM_TILE_FIREBOLT = 36,
  ITEM_TILE_FIRESURGE = 36,
  ITEM_TILE_FIRESTORM = 36,
  ITEM_TILE_FIRESPRAY = 36,
  ITEM_TILE_FIREPUSH = 36,
  ITEM_TILE_FIREJUMP = 36,
  ITEM_TILE_SPIRIT = 36,
  ITEM_TILE_FOOD = 37,
  ITEM_TILE_FLESH = 38,
  ITEM_TILE_POT = 39
};

static char item_string[ITEM_NUM][256] = {
  "empty",
  "firetorch",
  "firebolt",
  "firesurge",
  "firestorm",
  "firespray",
  "firepush",
  "firejump",
  "spirit orb",
  "cooked flesh",
  "raw flesh",
  "potion"
};

static int item_uses[ITEM_NUM] = {
  0, // empty
  0, // firetorch

  25,  // firebolt
  10,  // firesurge
  3,   // firestorm
  8,   // firespray
  15,  // firepush
  7,   // firejump
  10,  // spirit

  2, // food
  2, // flesh
  1, // pot
};

typedef struct {
  fvec2_t pos;
  ivec2_t tile;
  uint8_t item, active, uses;
} item_t;

#define ITEM_MAX 256

extern item_t item_list[ITEM_MAX];

void item_init();

void item_new(int x, int y, int type, int use);

item_t *item_take(int x, int y);

void item_render();

#endif // ITEM_H