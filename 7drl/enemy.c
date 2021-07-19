#include "enemy.h"
#include "player.h"
#include "text.h"
#include "main.h"
#include "math.h"
#include "spell.h"
#include "item.h"
#include "generator.h"

void skeleton_attack(entity_t *e, int first, int dist);
void goblin_attack(entity_t *e, int first, int dist);
void rat_attack(entity_t *e, int first, int dist);
void slime_attack(entity_t *e, int first, int dist);
void spider_attack(entity_t *e, int first, int dist);
void shamen_attack(entity_t *e, int first, int dist);
void bslime_attack(entity_t *e, int first, int dist);

void slime_die(entity_t *e);

void enemy_new(int enemy, int x, int y)
{
  entity_t *e = NULL;

  // we want the boss to render top-most
  if (enemy == ENEMY_TYPE_BOSS) {
    for (int i=ENTITY_MAX-1; i>0; i--) {
      if (!entity_list[i].alive) {
        memset(&entity_list[i], 0, sizeof(entity_t));
        e = &entity_list[i];
        break;
      }
    }
  } else {
    e = entity_new();
  }

  if (!e)
    return;

  e->update = NULL;
  e->onhit  = enemy_hit;
  e->ondeath= enemy_die;
  e->alive  = 1;
  e->speed  = 1;
  e->pos.x  = x;
  e->pos.y  = y;
  e->to.x   = x;
  e->to.y   = y;
  e->type   = enemy;
  e->dmap   = dmap_to_player;
  e->distance = -DIJ_MAX;

  // tile 16
  int tile = 0;

  switch(e->type) {
    case ENEMY_TYPE_SKELETON: {
      tile      = ENEMY_TILE_SKELETON;
      e->speed  = 1;
      e->update = enemy_update;
      e->attack = skeleton_attack;
      e->hp     = 8;
      e->lowhp  = 3;
      e->resist = 1;
      break;
    }
    case ENEMY_TYPE_GOBLIN: {
      tile      = ENEMY_TILE_GOBLIN;
      e->speed  = 2;
      e->update = enemy_update;
      e->attack = goblin_attack;
      e->hp     = 10;
      e->lowhp  = 4;
      e->resist = 0;
      break;
    }
    case ENEMY_TYPE_RAT: {
      tile      = ENEMY_TILE_RAT;
      e->speed  = 3;
      e->update = enemy_update;
      e->attack = rat_attack;
      e->hp     = 4;
      e->lowhp  = 0;
      e->resist = 0;
      e->cantopen = 1;
      break;
    }
    case ENEMY_TYPE_SLIME: {
      tile      = ENEMY_TILE_SLIME;
      e->speed  = 1;
      e->update = enemy_update;
      e->attack = slime_attack;
      e->ondeath= slime_die;
      e->hp     = 12;
      e->lowhp  = 0;
      e->resist = -1;
      break;
    }
    case ENEMY_TYPE_SPIDER: {
      tile      = ENEMY_TILE_SPIDER;
      e->speed  = 2;
      e->update = enemy_update;
      e->attack = spider_attack;
      e->hp     = 10;
      e->resist = -1;
      e->lowhp  = 3;
      e->distance = 3;
      break;
    }
    case ENEMY_TYPE_SHAMEN: {
      tile      = ENEMY_TILE_SHAMEN;
      e->speed  = 1;
      e->update = enemy_update;
      e->attack = shamen_attack;
      e->hp     = 20;
      e->resist = 1;
      e->lowhp  = 3;
      e->distance = 5;
      break;
    }
    case ENEMY_TYPE_BSLIME: {
      tile      = ENEMY_TILE_BSLIME;
      e->speed  = 2;
      e->update = enemy_update;
      e->attack = bslime_attack;
      e->hp     = 3;
      e->resist = 0;
      e->lowhp  = 0;
      e->cantopen = 1;
      break;
    }
    case ENEMY_TYPE_BOSS: {
      tile      = ENEMY_TILE_BOSS;
      e->speed  = 0;
      e->update = boss_update;
      e->attack = NULL;
      e->hp     = 100;
      e->resist = 2;
      e->lowhp  = 0;
      e->large  = 1;
      e->solid  = 0;
      e->cantopen = 1;
    }
  }

  e->hp += MAX(1, layer / 3) * 2;

  e->hp_max = e->hp;
  e->idistance = e->distance;
  entity_set_tile(e, 0, tile);
}

void boss_update(entity_t *e)
{
  if (!player->alive)
    return;

  // check LoS to player
  // collidable tiles
  int walls[32] = {-1};
    for (int i=0; i<SOLID_COUNT; i++)
      walls[i] = solid[i];
  walls[SOLID_COUNT-1] = TILE_DOOR_CLOSED;

  // raycast
  char *tiles = level.layers[level.layer].tiles;
  ivec2_t pos[512] = {0};

  int count = line(e->to.x, e->to.y, player->to.x, player->to.y, level_width, level_height, tiles, walls, pos);

  int d = hypot(e->to.x - player->to.x, e->to.y - player->to.y);

  // heal
  if (d > 10 && e->hp <= e->lowhp) {
    e->hp++;

    if (e->hp == e->lowhp)
      e->lowhp = 0;
  }

  if (pos[count-1].x == player->to.x && pos[count-1].y == player->to.y) {
    if (!e->aggro)
      text_log_add("The rat king notices you");
    e->aggro = 1;
  } else {
    e->aggro = 0;
  }

  if (e->aggro && roll(2) == 1) {
    enemy_new(ENEMY_TYPE_RAT, e->to.x + rand() % 2, e->to.y + rand() % 2);
    text_log_add("A rat breaks free from the rat king");
  }
}

void enemy_hit(entity_t *e, int damage, int type)
{
  if (!e->alive)
    return;

  char buff[512];
  switch(e->type) {
    case ENEMY_TYPE_SKELETON: {
      text_log_add("Skeleton resists some damage due to lack of flesh");
      sprintf(buff, "Skeleton suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_GOBLIN: {
      sprintf(buff, "Goblin suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_RAT: {
      sprintf(buff, "Rat suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_SLIME: {
      text_log_add("Slime ignites and takes increased damage");
      sprintf(buff, "Slime suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_SPIDER: {
      sprintf(buff, "Spider suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_SHAMEN: {
      sprintf(buff, "Shamen suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_BSLIME: {
      sprintf(buff, "Baby slime suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_BOSS: {
      sprintf(buff, "Rat king suffers %i damage", damage);
      break;
    }
  }

  e->aggro = 1;

  text_log_add(buff);
}

void enemy_die(entity_t *e)
{
  if (!e->alive)
    return;

  experience += MAX(1, layer / 3);
  pkills++;

  char buff[512];
  switch(e->type) {
    case ENEMY_TYPE_SKELETON: {
      text_log_add("The skeleton has died");
      break;
    }
    case ENEMY_TYPE_GOBLIN: {
      text_log_add("The goblin has died");
      if (roll(4) == 1)
        item_new(e->to.x, e->to.y, ITEM_FLESH, item_uses[ITEM_FLESH] * MAX(1, layer / 3));
      break;
    }
    case ENEMY_TYPE_RAT: {
      text_log_add("The rat has died");
      if (roll(2) == 1)
        item_new(e->to.x, e->to.y, ITEM_FLESH, item_uses[ITEM_FLESH] * MAX(1, layer / 3));
      break;
    }
    case ENEMY_TYPE_SLIME: {
      text_log_add("The slime has died");
      break;
    }
    case ENEMY_TYPE_SPIDER: {
      text_log_add("The spider has died");
      break;
    }
    case ENEMY_TYPE_SHAMEN: {
      text_log_add("The shaman has died");
      if (roll(4) == 1)
        item_new(e->to.x, e->to.y, ITEM_FLESH, item_uses[ITEM_FLESH] * MAX(1, layer / 3));
      if (roll(10) == 1)
        item_new(e->to.x, e->to.y, ITEM_SPIRIT, item_uses[ITEM_SPIRIT] * MAX(1, layer / 3));
      break;
    }
    case ENEMY_TYPE_BSLIME: {
      text_log_add("The baby slime has died");
      break;
    }
    case ENEMY_TYPE_BOSS: {
      text_log_add("The rat king has perished");
      text_log_add("Under the rat kings corpse you see an exit");
      update_chunk(e->to.x*tile_width, e->to.y*tile_height, TILE_EXIT);
      break;
    }
  }

  text_log_add(buff);
}

void enemy_update(entity_t *e)
{
  e->last_attack--;

  if (e->aggro)
    e->walking = 1;

  // set fleeing map if hurt
  if (e->hp <= e->lowhp) {
    e->dmap = dmap_from_player;
    e->distance = -DIJ_MAX;
    e->walking = 1;
  } else {
    e->dmap = dmap_to_player;
    e->distance = e->idistance;
  }

  if (!player->alive)
    return;

  // check LoS to player
  // collidable tiles
  int walls[32] = {-1};
    for (int i=0; i<SOLID_COUNT; i++)
      walls[i] = solid[i];
  walls[SOLID_COUNT-1] = TILE_DOOR_CLOSED;

  // raycast
  char *tiles = level.layers[level.layer].tiles;
  ivec2_t pos[512] = {0};

  int count = line(e->to.x, e->to.y, player->to.x, player->to.y, level_width, level_height, tiles, walls, pos);

  int d = hypot(e->to.x - player->to.x, e->to.y - player->to.y);

  // heal
  if (d > 10 && e->hp <= e->lowhp) {
    e->hp++;

    if (e->hp == e->lowhp)
      e->lowhp = 0;
  }

  if (pos[count-1].x == player->to.x && pos[count-1].y == player->to.y) {
    if (e->attack && !e->aggro)
      e->attack(e, 1, d);
    else if (e->attack)
      e->attack(e, 0, d);
    e->aggro = 1;
  }
}

void skeleton_attack(entity_t *e, int first, int dist)
{
  if (first)
    text_log_add("The skeleton notices you");

  if (e->aggro && e->last_attack <= 0 && dist <= 1) {
    player->hp -= 2;
    text_log_add("The skeleton hits you for 2 damage");
    e->last_attack = 2;
    heal_rate = HEALRATE;
  }
}

void goblin_attack(entity_t *e, int first, int dist)
{
  if (first)
    text_log_add("The goblin notices you");

  if (e->aggro && e->last_attack <= 0 && dist <= 1) {
    player->hp -= 2;
    text_log_add("The goblin hits you for 2 damage");
    e->last_attack = 2;
    heal_rate = HEALRATE;
  }
}

void rat_attack(entity_t *e, int first, int dist)
{
  if (first)
    text_log_add("The rat notices you");

  if (e->aggro && e->last_attack <= 0 && dist <= 1) {
    player->hp -= 1;
    text_log_add("The rat hits you for 1 damage");
    e->last_attack = 1;
    heal_rate = HEALRATE;
  }
}

void slime_attack(entity_t *e, int first, int dist)
{
  if (first)
    text_log_add("The slime notices you");

  if (e->aggro && e->last_attack <= 0 && dist <= 1) {
    player->hp -= 3;
    text_log_add("The slime hits you for 3 damage");
    e->last_attack = 2;
    heal_rate = HEALRATE;
  }
}

void spider_attack(entity_t *e, int first, int dist)
{

  if (first)
    text_log_add("The spider notices you");

  if (e->last_attack <= 0 && dist <= e->distance + 5) {
    int errx = 0;
    int erry = 0;
    if (roll(5) >= 3) {
      errx = -2 + roll(3);
      erry = -2 + roll(3);
    }
    spell_new(SPELL_WEB, e->to.x, e->to.y, player->to.x + errx, player->to.y + erry);
    e->last_attack = roll(2);
  }
}

void shamen_attack(entity_t *e, int first, int dist)
{

  if (first)
    text_log_add("The shamen notices you");

  if (e->aggro && e->last_attack <= 0 && dist <= e->distance + 5) {
    int errx = 0;
    int erry = 0;
    if (roll(2) == 1) {
      errx = -2 + roll(3);
      erry = -2 + roll(3);
    }
    spell_new(SPELL_SPIRIT, e->to.x, e->to.y, player->to.x + errx, player->to.y+ erry);
    e->last_attack = 3 + roll(3);
  }
}

void bslime_attack(entity_t *e, int first, int dist)
{
  if (first)
    text_log_add("The baby slime notices you");

  if (e->aggro && e->last_attack <= 0 && dist <= 1) {
    player->hp -= 1;
    text_log_add("The baby slime hits you for 1 damage");
    e->last_attack = 0;
    heal_rate = HEALRATE;
  }
}

void slime_die(entity_t *e)
{
  text_log_add("The slime breaks apart into multiple smaller slimes");

  int count = 0;
  for (int j=0; j<4; j++) {
    int tx = MAX(0, MIN(e->to.x + adjacent[j][0], level_width));
    int ty = MAX(0, MIN(e->to.y + adjacent[j][1], level_height));
    int tile = level.layers[level.layer].tiles[(ty*level_width)+tx];

    if (tx == e->to.x && ty == e->to.y)
      continue;

    if (tile != TILE_WOOD_FLOOR && tile != TILE_STONE_FLOOR && tile != TILE_DOOR_OPEN)
      continue;

    enemy_new(ENEMY_TYPE_BSLIME, tx, ty);
    count++;
  }

  for (int i=count; i<4; i++)
    enemy_new(ENEMY_TYPE_BSLIME, e->to.x, e->to.y);
}