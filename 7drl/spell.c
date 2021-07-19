#include "spell.h"
#include "main.h"
#include "entity.h"
#include "math.h"
#include "generator.h"
#include "player.h"
#include "text.h"
#include "item.h"

#define SPELL_MAX 512

uint8_t *fire_map = NULL, *fire_map_copy = NULL;

spell_t *spell_list = NULL;
ivec2_t fire_tile;

void spell_init()
{
  // initialize entity list
  if (spell_list)
    free(spell_list);
  spell_list = malloc(sizeof(spell_t) * SPELL_MAX);

  spell_t *spell;
  for (int i=0; i<SPELL_MAX; i++) {
    spell = &spell_list[i];
    spell->alive = 0;
  }

  if (fire_map)
    free(fire_map);
  if (fire_map_copy)
    free(fire_map_copy);
  fire_map = malloc(level_width * level_height);
  fire_map_copy = malloc(level_width * level_height);
  memset(fire_map, 0, level_width * level_height);
  memset(fire_map_copy, 0, level_width * level_height);

  ivec2_t t;
  int tw = tiles_tex_width  / rtile_width;
  picktile(&t, TILE_SPELL_FIRE, tw, rtile_width, rtile_height);
  memcpy(&fire_tile, &t, sizeof(ivec2_t));
}

int spell_get_range(int s)
{
  switch (s) {
    case SPELL_FIREBOLT: {
      return 10;
      break;
    }
    case SPELL_FIRESURGE: {
      return 8;
      break;
    }
    case SPELL_FIRESTORM: {
      return 20;
      break;
    }
    case SPELL_FIRESPRAY: {
      return 1;
      break;
    }
    case SPELL_FIREPUSH: {
      return 1;
      break;
    }

    case SPELL_FIREJUMP: {
      return 5;
      break;
    }
    case SPELL_WEB: {
      return 10;
      break;
    }
    case SPELL_SPIRIT: {
      return 50;
      break;
    }
    case CLOSE_DOOR: {
      return 1;
      break;
    }
    case DROP_ITEM: {
      return 0;
      break;
    }
    case FALL_DOWN: {
      return 1;
      break;
    }
  }

  return 0;
}

void spell_new(int s, int x0, int y0, int x1, int y1)
{
  switch (s) {
    case SPELL_FIRESURGE: {
      for (int j=0; j<8; j++) {
        if (roll(10) > 8)
          continue;

        int tx = MAX(0, MIN(x1 + around[j][0], level_width));
        int ty = MAX(0, MIN(y1 + around[j][1], level_height));
        int tile = level.layers[level.layer].tiles[(ty*level_width)+tx];

        if (tile != TILE_WOOD_FLOOR && tile != TILE_STONE_FLOOR && tile != TILE_DOOR_OPEN)
          continue;

        spell_newr(SPELL_FIREBOLT, x0, y0, tx, ty);
      }
      spell_newr(SPELL_FIREBOLT, x0, y0, x1, y1);
      return;
    }
    case SPELL_FIRESPRAY: {
      ivec2_t positions[512] = {0};

      // collidable tiles
      int walls[32] = {-1};
        for (int i=0; i<SOLID_COUNT; i++)
          walls[i] = solid[i];
      walls[SOLID_COUNT-1] = TILE_DOOR_CLOSED;

      char *tiles = level.layers[level.layer].tiles;
      float r = 8.5f;
      for (int j=0; j<360; j+=8) {
        float fromx = x0 + 0.5f;
        float fromy = y0 + 0.5f;
        float tox = fromx + (r * sintable[j]);
        float toy = fromy + (r * costable[j]);

        int count = line(fromx, fromy, tox, toy, level_width, level_height, tiles, walls, positions);

        for (int k=1; k<count-1; k++) {
          entity_t *e;
          int hit = 0;
          for (int i=0; i<ENTITY_MAX; i++) {
            e = &entity_list[i];

            if (e->alive && e->to.x == positions[k].x && e->to.y == positions[k].y) {
              hit = 1;
              break;
            }
          }

          if (hit || k == count-2) {
            spell_newr(SPELL_FIRESPRAY, x0, y0, positions[k].x, positions[k].y);
            break;
          }
        }
      }
      return;
    }
  }

  spell_newr(s, x0, y0, x1, y1);
}

void spell_newr(int s, int x0, int y0, int x1, int y1)
{
  // get a new spell instance
  spell_t *spell = NULL;
  for (int i=0; i<SPELL_MAX; i++) {
    if (!spell_list[i].alive) {
      memset(&spell_list[i], 0, sizeof(spell_t));
      spell = &spell_list[i];
      break;
    }
  }

  // spell list overflow
  if (!spell)
    return;

  // set spell specific stats
  spell->spell = s;
  spell->alive = 1;
  spell->lit   = 1;
  int tile = 0;
  switch (s) {
    case SPELL_FIREBOLT: {
      spell->range      = spell_get_range(s);
      spell->firechance = 7;
      spell->speed      = 20;
      spell->damage     = 4;
      tile = TILE_SPELL_FIREBOLT;
      break;
    }
    case SPELL_FIRESTORM: {
      spell->range      = spell_get_range(s);
      spell->firechance = 1;
      spell->speed      = 20;
      spell->damage     = 15;
      tile = TILE_SPELL_FIRESTORM;
      break;
    }
    case SPELL_FIRESPRAY: {
      spell->range      = spell_get_range(s);
      spell->firechance = 6;
      spell->speed      = 15;
      spell->damage     = 2;
      tile = TILE_SPELL_FIRESPRAY;
      break;
    }
    case SPELL_WEB: {
      spell->range      = spell_get_range(s);
      spell->firechance = 0;
      spell->speed      = 30;
      spell->damage     = 0;
      spell->lit        = 0;
      tile = TILE_SPELL_WEB;
      break;
    }
    case SPELL_SPIRIT: {
      spell->range      = spell_get_range(s);
      spell->firechance = 0;
      spell->speed      = 10;
      spell->damage     = 3;
      spell->lit        = 1;
      tile = TILE_SPELL_SPIRIT;
      break;
    }
  }

  ivec2_t t;
  int tw = tiles_tex_width  / rtile_width;
  picktile(&t, tile, tw, rtile_width, rtile_height);
  memcpy(&spell->tile, &t, sizeof(ivec2_t));

  // set to position to wall hit
  spell->pos.x  = x0;
  spell->pos.y  = y0;
  spell->from.x = x0;
  spell->from.y = y0;
  spell->t      = 0.0f;
  spell->to.x   = x1;
  spell->to.y   = y1;
  spell->update = 0;
}

void spell_update()
{
  spell_t *spell;
  for (int i=0; i<SPELL_MAX; i++) {
    spell = &spell_list[i];

    if (!spell->alive)
      continue;

    if (!spell->update)
      spell->update = 1;
  }

  memcpy(fire_map_copy, fire_map, level_width * level_height);
  for (int y=0; y<level_height; y++) {
    for (int x=0; x<level_width; x++) {
      int index = (y*level_width)+x;

      if (!fire_map_copy[index])
        continue;

      // fire spread
      for (int i=0; i<4; i++) {
        int tx = MAX(0, MIN(x + adjacent[i][0], level_width));
        int ty = MAX(0, MIN(y + adjacent[i][1], level_height));
        if (tx == x && ty == y)
          continue;

        int index2 = (ty*level_width)+tx;
        if (fire_map[index2])
          continue;

        if (roll(10) == 1)
          continue;

        int t = check_tile(tx, ty);
        int spread = fire_map[index] / 2;
        if (t == TILE_WOOD_FLOOR || t == TILE_DOOR_OPEN) {
          if (roll(10) > 1)
            fire_map[index2] = MAX(0, fire_map_copy[index] - 3);
          else
            fire_map[index2] = fire_map_copy[index];
        } else if (t == TILE_STONE_FLOOR) {
          fire_map[index2] = MAX(0, fire_map_copy[index] - 4);
        }
      }

      for (int i=0; i<ITEM_MAX; i++) {
        if (item_list[i].active && item_list[i].pos.x == x && item_list[i].pos.y == y) {
          if (item_list[i].item == ITEM_FLESH) {
            item_list[i].item = ITEM_FOOD;
            text_log_add("The fire cooks the raw flesh");
            ivec2_t t;
            int tw = tiles_tex_width  / rtile_width;
            picktile(&t, ITEM_TILE_FOOD, tw, rtile_width, rtile_height);
            memcpy(&item_list[i].tile, &t, sizeof(ivec2_t));
          } else if (item_list[i].item != ITEM_FOOD) {
            if (roll(5) == 1)
              item_list[i].active = 0;
          }
        }
      }

      entity_hit(x, y, FIRE_DAMAGE, -1);
      fire_map[index]--;

      // break floor
      int t = check_tile(x, y);
      if (!fire_map[index] && roll(FIRE_BREAK_CHANCE) == 1 && t == TILE_WOOD_FLOOR) {
        update_chunk(x * tile_width, y * tile_height, TILE_WOOD_HOLE);
        for (int i=0; i<ITEM_MAX; i++) {
          if (item_list[i].pos.x == x && item_list[i].pos.y == y)
            item_list[i].active = 0;
        }
      }
    }
  }
}

int spell_ready()
{
  spell_t *spell;
  for (int i=0; i<SPELL_MAX; i++) {
    spell = &spell_list[i];

    if (!spell->alive)
      continue;

    if (spell->update)
      return 0;
  }

  return 1;
}

void spell_hit(spell_t *spell)
{
  // handle enemy hits etc
  // set fires etc

  // set fires
  int tx = floor(spell->to.x);
  int ty = floor(spell->to.y);
  int tindex = (ty*level_width)+tx;
  if (spell->firechance && roll(spell->firechance) == 1) {
    int tile = check_tile(tx, ty);
    if (tile == TILE_WOOD_FLOOR)
      fire_map[tindex] = 8 + roll(6);
    else
      fire_map[tindex] = 5 + roll(3);
  }


  // collidable tiles
  int walls[32] = {-1};
    for (int i=0; i<SOLID_COUNT; i++)
      walls[i] = solid[i];
  walls[SOLID_COUNT] = TILE_DOOR_CLOSED;

  char *tiles = level.layers[level.layer].tiles;
  ivec2_t positions[512] = {0};

  switch (spell->spell) {
    case SPELL_FIRESTORM: {
      for (int i=0; i<48; i++) {
        int x = -4 + roll(7);
        int y = -4 + roll(7);
        spell_new(SPELL_FIREBOLT, spell->to.x-x, spell->to.y-30-y, spell->to.x+x, spell->to.y+y);
      }
      break;
    }
  }

  entity_hit(tx, ty, spell->damage + plevel, spell->spell);

  spell->alive = 0;
}

void spell_update_render()
{
  player_light();

  ivec2_t positions[512] = {0};

  // collidable tiles
  int walls[32] = {-1};
    for (int i=0; i<SOLID_COUNT; i++)
      walls[i] = solid[i];
  walls[SOLID_COUNT] = TILE_DOOR_CLOSED;

  char *tiles = level.layers[level.layer].tiles;

  // update lighting based on fire map
  int fx = MAX(0, floor(cx / tile_width) - 5);
  int fy = MAX(0, floor(cy / tile_height) - 5);
  int tx = MAX(0, MIN(fx + (window_width / tile_width) + 5, level_width));
  int ty = MAX(0, MIN(fy + (game_height / tile_height) + 5, level_height));
  for (int y=fy; y<ty; y++) {
    for (int x=fx; x<tx; x++) {
      int index = (y*level_width)+x;
      if (!fire_map[index])
        continue;

      // handle lighting
      float r = 2.5f;
      for (int j=0; j<360; j++) {
        float fromx = x + 0.5f;
        float fromy = y + 0.5f;
        float tox = fromx + (r * sintable[j]);
        float toy = fromy + (r * costable[j]);

        int count = line(fromx, fromy, tox, toy, level_width, level_height, tiles, walls, positions);

        for (int k=0; k<count; k++) {
          if (check_tile(positions[k].x, positions[k].y) == TILE_NONE)
            continue;
          float val = MIN((k / r), 1.0f);
          if (roll(2) == 1)
            val = MIN(val + 0.3f, 1.0f);
          int vala  = 100 * val;
          int valb  = 50 * val;
          int tindex = ((positions[k].y*level_width)+positions[k].x)*4;
          light_map[tindex+0] = 200 - vala;
          light_map[tindex+1] = 150 - vala;
          light_map[tindex+2] = 0;
          light_map[tindex+3] = 80 - valb;
        }
      }
    }
  }

  spell_t *spell;
  for (int i=0; i<SPELL_MAX; i++) {
    spell = &spell_list[i];

    if (!spell->alive || !spell->update)
      continue;

    float distance = 1.0f / hypot(spell->from.x - spell->to.x, spell->from.y - spell->to.y);
    spell->t += delta_time;
    spell->step = (spell->t * distance) * spell->speed;

    // lerp positions
    spell->pos.x = lerp(spell->step, spell->from.x, spell->to.x);
    spell->pos.y = lerp(spell->step, spell->from.y, spell->to.y);

    if (spell->step >= 1.0f) {
      spell_hit(spell);
      continue;
    }

    if (!spell->lit)
      continue;

    // handle lighting
    float r = 5.5f - 4.0f * (spell->step);
    for (int j=0; j<360; j++) {
      float fromx = spell->pos.x + 0.5f;
      float fromy = spell->pos.y + 0.5f;
      float tox = fromx + (r * sintable[j]);
      float toy = fromy + (r * costable[j]);

      int count = line(fromx, fromy, tox, toy, level_width, level_height, tiles, walls, positions);

      for (int k=0; k<count; k++) {
        if (check_tile(positions[k].x, positions[k].y) == TILE_NONE)
          continue;
        float val = MIN((k / r), 1.0f);
        if (roll(2) == 1)
          val = MIN(val + 0.15f, 1.0f);
        int vala  = 100 * val;
        int valb  = 200 * val;
        int index = ((positions[k].y*level_width)+positions[k].x)*4;
        light_map[index+0] = 220 - vala;
        light_map[index+1] = 220 - valb;
        light_map[index+2] = 0;
        light_map[index+3] = 130 - vala;
      }
    }
  }
}

void spell_render()
{
  spell_t *spell;
  SDL_Rect ra, rb;
  ra.w = rtile_width;
  ra.h = rtile_height;
  rb.w = tile_width;
  rb.h = tile_height;
  for (int i=0; i<SPELL_MAX; i++) {
    spell = &spell_list[i];

    if (!spell->alive)
      continue;

    SDL_SetTextureAlphaMod(tex_tiles, 120);

    ra.x = spell->tile.x;
    ra.y = spell->tile.y;
    rb.x = (spell->pos.x*tile_width)-cx;
    rb.y = (spell->pos.y*tile_height)-cy;
    SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);

    // render a reflection
    if (check_tile(spell->to.x, spell->to.y) == TILE_STONE_FLOOR &&
        check_tile(spell->to.x, spell->to.y+1) == TILE_STONE_FLOOR) {
      SDL_SetTextureAlphaMod(tex_tiles, 80);
      rb.y += (tile_height);
      SDL_RenderCopyEx(renderer, tex_tiles, &ra, &rb, 0, NULL, SDL_FLIP_VERTICAL);
    }
  }
}

void spell_render_fire()
{
  // render fire
  spell_t *spell;
  SDL_Rect ra, rb;
  ra.w = rtile_width;
  ra.h = rtile_height;
  rb.w = tile_width;
  rb.h = tile_height;
  int fx = MAX(0, floor(cx / tile_width) - 1);
  int fy = MAX(0, floor(cy / tile_height) - 1);
  int tx = MAX(0, MIN(fx + (window_width / tile_width) + 2, level_width));
  int ty = MAX(0, MIN(fy + (game_height / tile_height) + 2, level_height));
  for (int y=fy; y<ty; y++) {
    for (int x=fx; x<tx; x++) {
      int index = (y*level_width)+x;
      if (!fire_map[index])
        continue;

      SDL_SetTextureAlphaMod(tex_tiles, 50 + roll(50));

      ra.x = fire_tile.x;
      ra.y = fire_tile.y;
      rb.x = (x*tile_width)-cx;
      rb.y = (y*tile_height)-cy;
      SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);
    }
  }

  SDL_SetTextureAlphaMod(tex_tiles, 255);
}
