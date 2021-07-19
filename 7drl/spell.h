#ifndef SPELL_H
#define SPELL_H

#include "entity.h"
#include <inttypes.h>

#define FIRE_DAMAGE 1
#define FIRE_BREAK_CHANCE 10

enum {
  SPELL_FIREBOLT,
  SPELL_FIRESURGE,
  SPELL_FIRESTORM,
  SPELL_FIRESPRAY,
  SPELL_FIREPUSH,
  SPELL_FIREJUMP,
  SPELL_WEB,
  SPELL_SPIRIT,

  SPELL_NUM,

  CLOSE_DOOR,
  DROP_ITEM,
  FALL_DOWN
};

enum {
  TILE_SPELL_FIREBOLT = 32,
  TILE_SPELL_FIRESURGE = 32,
  TILE_SPELL_FIRESTORM = 32,
  TILE_SPELL_FIRESPRAY = 32,
  TILE_SPELL_FIRE = 33,
  TILE_SPELL_WEB = 34,
  TILE_SPELL_SPIRIT = 35,
};

typedef struct {
  fvec2_t pos, to, from;
  ivec2_t tile;
  float t, step;

  int range, firechance, speed;
  int damage, update;
  int alive, spell;
  int lit;
  int *dmap;
} spell_t;

extern uint8_t *fire_map;

int spell_get_range(int s);

void spell_init();

void spell_new(int s, int x0, int y0, int x1, int y1);

void spell_newr(int s, int x0, int y0, int x1, int y1);

void spell_update();

int spell_ready();

void spell_hit(spell_t *spell);

void spell_update_render();

void spell_render();

void spell_render_fire();

#endif // SPELL_H