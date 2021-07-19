#ifndef MAIN_H
#define MAIN_H

#include <SDL2/SDL.h>

#define CHUNK_STRIDE 16       // width of chunk array
#define CHUNK_COUNT  (16*16)  // max amount of chunks we will need
#define CHUNK_WIDTH  512      // size in pixels
#define CHUNK_HEIGHT 528      // size in pixels
#define MAX_LAYER 10

extern SDL_Window *window;
extern SDL_Renderer *renderer;
static const int window_width  = 840; // 960
static const int window_height = 600; // 769
static const int game_height   = 600; // 648
static const int tile_width    = 16;
static const int tile_height   = 24;
static const int rtile_width   = 16;
static const int rtile_height  = 24;

extern double delta_time, tick;
extern float camx, camy, cx, cy, zoom;
extern int mx, my;

extern int update;

extern int tiles_tex_width, tiles_tex_height;

extern SDL_Texture *tex_tiles;

extern uint8_t keys_down[SDL_NUM_SCANCODES];

enum {
  TILEI_NONE = 0,
  TILEI_PLAYER = 16,
  TILEI_SPELL  = 32
};

typedef struct {
  size_t width, height, sx, sy, ex, ey;
  char *tiles;
} map_t;

typedef struct {
  map_t layers[1];
  int layer, max;
} level_t;

// each chunk is 512x512px
// so, 32x32 tiles of 16x16px each
typedef struct {
  SDL_Texture *tex;
  int update, tile_count;
  int updated[64], count;
} level_chunk_t;

typedef struct {
  level_chunk_t chunks[CHUNK_COUNT];
} level_texture_t;

extern level_t level;
extern level_texture_t level_textures;
extern int level_width, level_height;
extern int layer;

void godown();

static void update_chunk(int x, int y, int tile)
{
  int tx = x / tile_width;
  int ty = y / tile_height;
  int index = (ty*level_width)+tx;
  if (level.layers[level.layer].tiles[index] == tile)
    return;
  level.layers[level.layer].tiles[index] = tile;

  int cx = floor(x / CHUNK_WIDTH);
  int cy = floor(y / CHUNK_HEIGHT);
  level_chunk_t *chunk = &level_textures.chunks[(cy*CHUNK_STRIDE)+cx];
  chunk->update = 1;
  chunk->updated[chunk->count++] = index;
}

static int check_tile(int x, int y)
{
  return level.layers[level.layer].tiles[(y*level_width)+x];
}

#endif // MAIN_H