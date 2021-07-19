#ifndef ENTITY_H
#define ENTITY_H

#include "math.h"

#define ENTITY_MAX 256

typedef struct {
  fvec2_t pos, to;
  ivec2_t tiles[2];
  int alive, animated, frame;
  int updating, hp, hp_max, lowhp, resist;
  int cantopen, large, solid;

  int walking, speed, type, aggro;
  int last_attack, stuck;
  int distance, idistance; // preferred tile value on dmap
  int *dmap;

  void (*onhit)(void *e, int damage, int type);
  void (*update)(void *e);
  void (*ondeath)(void *e);
  void (*attack)(void *e, int first, int dist);
} entity_t;

extern entity_t *entity_list;

void entity_init();

void entity_hit(int x, int y, int damage, int type);

void entity_push(int x, int y, int dx, int dy);

void entity_update();

int entity_ready();

void entity_update_render();

entity_t *entity_new();

void entity_set_tile(entity_t *e, int i, size_t index);

void entity_render();

#endif // ENTITY_H