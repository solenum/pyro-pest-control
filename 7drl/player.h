#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"
#include <inttypes.h>

#define HEALRATE 8
#define INVENTORY_MAX 6

extern entity_t *player;
extern int light_map_width, light_map_height;
extern uint8_t *light_map;
extern int *dmap_to_player, *dmap_from_player, *dmap_to_mouse;

extern int inventory[INVENTORY_MAX];
extern int uses[INVENTORY_MAX];

extern int heal_rate;

extern int experience;
extern int plevel;
extern int pkills;

void player_reinit();

void player_init(int x, int y);

void player_die(entity_t *e);

void player_hit(entity_t *e, int damage, int type);

void player_light();

void player_fire();

int player_update();

void player_render();

void player_keypress(int button);

void player_mousepress(int button, int mx, int my);

#endif // PLAYER_H