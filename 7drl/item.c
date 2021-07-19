#include "main.h"
#include "item.h"

item_t item_list[ITEM_MAX];

void item_init()
{
  memset(item_list, 0, sizeof(item_t) * ITEM_MAX);
}

void item_new(int x, int y, int type, int use)
{
  item_t *item = NULL;
  for (int i=0; i<ITEM_MAX; i++) {
    if (!item_list[i].active) {
      item = &item_list[i];
      break;
    }
  }

  if (!item)
    return;

  item->pos.x  = x;
  item->pos.y  = y;
  item->item   = type;
  item->active = 1;
  item->uses   = use;

  int tile;
  switch (type) {
    case ITEM_FIRETORCH: {

      tile = ITEM_TILE_FIRETORCH;
      break;
    }
    case ITEM_FIREBOLT: {

      tile = ITEM_TILE_FIREBOLT;
      break;
    }
    case ITEM_FIRESURGE: {

      tile = ITEM_TILE_FIRESURGE;
      break;
    }
    case ITEM_FIRESTORM: {

      tile = ITEM_TILE_FIRESTORM;
      break;
    }
    case ITEM_FIRESPRAY: {

      tile = ITEM_TILE_FIRESPRAY;
      break;
    }
    case ITEM_FIREPUSH: {

      tile = ITEM_TILE_FIREPUSH;
      break;
    }
    case ITEM_FIREJUMP: {

      tile = ITEM_TILE_FIREJUMP;
      break;
    }
    case ITEM_SPIRIT: {

      tile = ITEM_TILE_SPIRIT;
      break;
    }
    case ITEM_FOOD: {

      tile = ITEM_TILE_FOOD;
      break;
    }
    case ITEM_FLESH: {

      tile = ITEM_TILE_FLESH;
      break;
    }
    case ITEM_POT: {

      tile = ITEM_TILE_POT;
      break;
    }
  }

  ivec2_t t;
  int tw = tiles_tex_width  / rtile_width;
  picktile(&t, tile, tw, rtile_width, rtile_height);
  memcpy(&item->tile, &t, sizeof(ivec2_t));
}

item_t *item_take(int x, int y)
{
  for (int i=0; i<ITEM_MAX; i++) {
    item_t *item = &item_list[i];

    if (!item->active)
      continue;

    if (item->pos.x == x && item->pos.y == y) {
      item->active = 0;
      return item;
    }
  }

  return NULL;
}

void item_render()
{
  item_t *item;
  SDL_Rect ra, rb;
  ra.w = rtile_width;
  ra.h = rtile_height;
  rb.w = tile_width;
  rb.h = tile_height;
  for (int i=0; i<ITEM_MAX; i++) {
    item = &item_list[i];

    if (!item->active)
      continue;

    float ix = (item->pos.x*tile_width)-cx;
    float iy = (item->pos.y*tile_height)-cy;
    if (ix < 0 || iy < 0 || ix > window_width || iy > window_height)
      continue;

    ra.x = item->tile.x;
    ra.y = item->tile.y;
    rb.x = (item->pos.x*tile_width)-cx;
    rb.y = (item->pos.y*tile_height)-cy;
    SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);
  }
}