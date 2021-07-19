#include <stdio.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <emscripten/emscripten.h>

#include "main.h"
#include "math.h"
#include "text.h"
#include "texture.h"
#include "generator.h"
#include "entity.h"
#include "player.h"
#include "spell.h"
#include "enemy.h"
#include "item.h"

// sdl vars
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

// camera vars
float camx = 512.0f, camy = 512.0f;
float cx = 0.0f, cy = 0.0f;
int mx, my;

// runs an update pass on all entites when set
int update = 0;

// delta time vars
const double phys_delta_time   = 1.0 / 60.0;
const double slowest_frame     = 1.0 / 15.0;
double delta_time, tick        = 0.0;
double last_frame_time         = 0.0;
int init = 0, reinit = 0, goingdown = 0;

// level vars
SDL_Texture *tex_tiles = NULL, *tex_map = NULL, *tex_fov = NULL;
SDL_Texture *tex_menu = NULL;
int tiles_tex_width = 0, tiles_tex_height = 0;
level_t level;
level_texture_t level_textures;
int level_width = 0, level_height = 0;
int layer = 0;
int menu = 1;
int menuinout = 0;
float menualpha = -0.2f;

// input
uint8_t keys_down[SDL_NUM_SCANCODES];
void keypressed(int key);
void input(SDL_Event *event);

void pickspawn(int *xpos, int *ypos, int dist)
{
  for (int i=0; i<10000; i++) {
    int x = rand() % level_width;
    int y = rand() % level_height;

    int tile = level.layers[level.layer].tiles[(y*level_width)+x];

    if (tile == TILE_WOOD_FLOOR || tile == TILE_STONE_FLOOR) {
      int d = hypot(x - player->to.x, y - player->to.y);
      if (d >= dist) {
        *xpos = x;
        *ypos = y;
        return;
      }
    }
  }
}

void spawn()
{
  // enemy spawns
  int count = MIN((layer+2) * 5, 50) + roll(MAX(layer, 1));
  int sx, sy;
  for (int i=0; i<count; i++) {
    if (layer < 3) {
      // skeleton goblin rat
      int types[] = {
        ENEMY_TYPE_SKELETON,
        ENEMY_TYPE_SKELETON,
        ENEMY_TYPE_GOBLIN,
        ENEMY_TYPE_RAT,
        ENEMY_TYPE_RAT
      };
      int enemy = types[rand() % 5];
      pickspawn(&sx, &sy, 10);
      enemy_new(enemy, sx, sy);
    }
    else if (layer >= 3 && layer <= 6) {
      // skeleton goblin rat slime spider
      int types[] = {
        ENEMY_TYPE_SKELETON,
        ENEMY_TYPE_SKELETON,
        ENEMY_TYPE_GOBLIN,
        ENEMY_TYPE_GOBLIN,
        ENEMY_TYPE_RAT,
        ENEMY_TYPE_SLIME,
        ENEMY_TYPE_SLIME,
        ENEMY_TYPE_SPIDER
      };
      int enemy = types[rand() % 8];
      pickspawn(&sx, &sy, 10);
      enemy_new(enemy, sx, sy);
    } else {
      // skeleton goblin rat slime spider shaman
      int types[] = {
        ENEMY_TYPE_SKELETON,
        ENEMY_TYPE_GOBLIN,
        ENEMY_TYPE_GOBLIN,
        ENEMY_TYPE_RAT,
        ENEMY_TYPE_RAT,
        ENEMY_TYPE_SLIME,
        ENEMY_TYPE_SLIME,
        ENEMY_TYPE_SPIDER,
        ENEMY_TYPE_SPIDER,
        ENEMY_TYPE_SPIDER,
        ENEMY_TYPE_SHAMEN,
        ENEMY_TYPE_SHAMEN,
        ENEMY_TYPE_SHAMEN
      };
      int enemy = types[rand() % 13];
      pickspawn(&sx, &sy, 8);
      enemy_new(enemy, sx, sy);

      if (layer == MAX_LAYER && roll(10) == 1)
        enemy_new(ENEMY_TYPE_RAT, sx, sy);
    }
  }

  // spawn boss
  if (layer > MAX_LAYER) {
    for (;;) {
      int x = 1 + rand() % level_width-2;
      int y = 1 + rand() % level_height-2;

      int tilea = level.layers[level.layer].tiles[(y*level_width)+x];
      int tileb = level.layers[level.layer].tiles[(y*level_width)+x+1];
      int tilec = level.layers[level.layer].tiles[((y+1)*level_width)+x];
      int tiled = level.layers[level.layer].tiles[((y+1)*level_width)+x+1];

      if (tilea == TILE_WOOD_FLOOR && tileb == TILE_WOOD_FLOOR && tilec == TILE_WOOD_FLOOR && tiled == TILE_WOOD_FLOOR) {
        int d = hypot(x - player->to.x, y - player->to.y);
        if (d >= 15) {
          enemy_new(ENEMY_TYPE_BOSS, x, y);
          break;
        }
      }
    }
  }

  count = MAX(3, (23 - layer) - (layer) + roll(3));
  for (int i=0; i<count; i++) {
    if (layer < 3) {
      int items[] = {
        ITEM_FIREBOLT,
        ITEM_FIRESURGE,
        ITEM_FIREPUSH,
        ITEM_POT,
        ITEM_POT,
        ITEM_POT,
        ITEM_FLESH
      };
      int item = items[rand() % 7];
      pickspawn(&sx, &sy, 5);
      int use_mod = MAX(1, layer / 3);
      item_new(sx, sy, item, item_uses[item] * use_mod);
    } else if (layer >= 3 && layer <= 5) {
      int items[] = {
        ITEM_FIREBOLT,
        ITEM_FIREBOLT,
        ITEM_FIRESURGE,
        ITEM_FIRESURGE,
        ITEM_FIRESPRAY,
        ITEM_FIRESPRAY,
        ITEM_FIREPUSH,
        ITEM_FIREPUSH,
        ITEM_FIRESTORM,
        ITEM_FIREJUMP,
        ITEM_FIREJUMP,
        ITEM_POT,
        ITEM_POT,
        ITEM_FLESH,
        ITEM_FLESH
      };
      int item = items[rand() % 15];
      pickspawn(&sx, &sy, 8);
      int use_mod = MAX(1, layer / 3);
      item_new(sx, sy, item, item_uses[item] * use_mod);
    } else {
      int items[] = {
        ITEM_FIREBOLT,
        ITEM_FIREBOLT,
        ITEM_FIREBOLT,
        ITEM_FIREBOLT,
        ITEM_FIRESURGE,
        ITEM_FIRESURGE,
        ITEM_FIRESURGE,
        ITEM_FIRESPRAY,
        ITEM_FIRESPRAY,
        ITEM_FIRESPRAY,
        ITEM_FIREPUSH,
        ITEM_FIREPUSH,
        ITEM_FIREPUSH,
        ITEM_FIRESTORM,
        ITEM_FIREJUMP,
        ITEM_FIREJUMP,
        ITEM_FIREJUMP,
        ITEM_POT,
        ITEM_FLESH
      };
      int item = items[rand() % 19];
      pickspawn(&sx, &sy, 10);
      int use_mod = MAX(1, layer / 3);
      item_new(sx, sy, item, item_uses[item] * use_mod);
    }
  }
}

void godown()
{
  // end game
  if (layer > MAX_LAYER) {
    char buff[256];
    text_log_add("You have defeated the rat king and escaped");
    sprintf(buff, "You where level %i", plevel);
    text_log_add(buff);
    sprintf(buff, "You have slain %i", pkills);
    text_log_add(buff);
    player->alive = 0;
    return;
  }

  update = 0;

  // generate the level
  level.layer = 0;
  level.max   = 10 + roll(10);
  while (!gen(&level.layers[0])) {};
  level_width  = level.layers[level.layer].width;
  level_height = level.layers[level.layer].height;

  item_init();

  // reinit entity stuff
  entity_init();

  player->alive = 1;
  player->pos.x = level.layers[level.layer].sx;
  player->pos.y = level.layers[level.layer].sy;
  player->to.x  = level.layers[level.layer].sx;
  player->to.y  = level.layers[level.layer].sy;

  player_reinit();

  text_log_add("You descend further into the dungeon");

  if (layer == 3)
    text_log_add("You hear a scuttle in the distance");
  if (layer == 7)
    text_log_add("You sense an evil presence");

  // set exit
  if (layer < MAX_LAYER) {
    level.layers[level.layer].tiles[(level.layers[level.layer].ey*level_width)+level.layers[level.layer].ex] = TILE_EXIT;
  } else {
    text_log_add("You sense the presence of a powerful creature");
  }

  for (int i=0; i<CHUNK_COUNT; i++) {
    level_textures.chunks[i].update     = 1;
    level_textures.chunks[i].tile_count = 0;
    level_textures.chunks[i].count      = 0;
  }

  SDL_SetTextureAlphaMod(tex_font, 255);
  SDL_SetTextureAlphaMod(tex_tiles, 255);
  SDL_SetTextureAlphaMod(tex_map, 255);
  SDL_SetTextureAlphaMod(tex_fov, 255);

  // reinit spell system
  spell_init();

  layer++;

  spawn();
}

void start()
{
  // generate the level
  level.layer = 0;
  level.max   = 10 + roll(10);
  while (!gen(&level.layers[0])) {};
  level_width  = level.layers[level.layer].width;
  level_height = level.layers[level.layer].height;

  layer = 0;
  update = 0;

  item_init();

  // init entity stuff
  entity_init();

  // init player
  player_init(level.layers[level.layer].sx, level.layers[level.layer].sy);

  // set exit
  level.layers[level.layer].tiles[(level.layers[level.layer].ey*level_width)+level.layers[level.layer].ex] = TILE_EXIT;

  spawn();

  // init spell system
  spell_init();

  // setup vga atlas etc
  text_init();
}

void loop()
{
  /*-----------------------------------------/
  /---------------- INIT STUFF --------------/
  /-----------------------------------------*/
  if (!init) {
    init++;

    if (!reinit) {
      // init SDL
      if (!SDL_Init(SDL_INIT_EVERYTHING)) {
        printf("Failed to init SDL2\n");
        return;
      }

      // init window
      window = SDL_CreateWindow("rl", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_GRABBED);

      // init renderer
      renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

      SDL_CaptureMouse(SDL_TRUE);
      SDL_SetWindowInputFocus(window);

      // nearest filtering
      SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

      // dt stuff
      last_frame_time = SDL_GetPerformanceCounter();

      // load textures
      tex_tiles = texture_load("tiles.png", &tiles_tex_width, &tiles_tex_height);
      SDL_SetTextureBlendMode(tex_tiles, SDL_BLENDMODE_BLEND);

      // load menu bg
      tex_menu = texture_load("cover.png", NULL, NULL);
      SDL_SetTextureBlendMode(tex_menu, SDL_BLENDMODE_BLEND);

      // screen render target
      tex_map = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, window_width, game_height);

      // field of view light map
      tex_fov = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, (window_width / tile_width) + 2, (game_height / tile_height) + 2);
      SDL_SetTextureBlendMode(tex_fov, SDL_BLENDMODE_BLEND);

      for (int i=0; i<CHUNK_COUNT; i++) {
        level_textures.chunks[i].tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, CHUNK_WIDTH, CHUNK_HEIGHT);
      }

      level.layers[0].tiles = NULL;
    }

    for (int i=0; i<CHUNK_COUNT; i++) {
      level_textures.chunks[i].update     = 1;
      level_textures.chunks[i].tile_count = 0;
      level_textures.chunks[i].count      = 0;
    }

    start();

    reinit++;
  }
  /*----------------------------------------*/



  /*-----------------------------------------/
  /---------------- UPDATE LOGIC ------------/
  /-----------------------------------------*/
  // calculate delta time
  double current_frame_time = (double)SDL_GetPerformanceCounter();
  delta_time = (double)(current_frame_time - last_frame_time) / (double)SDL_GetPerformanceFrequency();
  last_frame_time = current_frame_time;

  // prevent spiral of death
  if (delta_time > slowest_frame)
    delta_time = slowest_frame;

  tick += delta_time;

  // handle sdl events
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      // cya
      case SDL_QUIT:
        return;
        break;

      // input events
      case SDL_KEYDOWN:
      case SDL_KEYUP:
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
      case SDL_MOUSEWHEEL:
      case SDL_TEXTEDITING:
      case SDL_TEXTINPUT:
      case SDL_MOUSEMOTION:
      case SDL_KEYMAPCHANGED: {
        input(&event);
        break;
      }

      // window events
      case SDL_WINDOWEVENT: {
        break;
      }
    }
  }

  // update mouse coords
  if (SDL_GetRelativeMouseMode())
    SDL_GetRelativeMouseState(&mx, &my);
  else
    SDL_GetMouseState(&mx, &my);

  if (player != NULL && player->alive) {
    camx += ((player->to.x*tile_width) - camx) * 8.0f * delta_time;
    camy += ((player->to.y*tile_height) - camy) * 8.0f * delta_time;
  }

  // repeat until all are done updating
  if (update && tick > 1.0f / 16.0f) {
    if (spell_ready()) {
      entity_update();
      spell_update();
      update = player_update();
      tick = 0.0f;
    }
  }
  entity_update_render();
  spell_update_render();
  /*----------------------------------------*/



  /*-----------------------------------------/
  /---------------- TILE MAP CHUNK RENDER ---/
  /-----------------------------------------*/
  for (int iy=0; iy<CHUNK_STRIDE; iy++) {
    for (int ix=0; ix<CHUNK_STRIDE; ix++) {
      int index = (iy*CHUNK_STRIDE)+ix;

      if (!level_textures.chunks[index].update)
        continue;

      // num of tiles that need updating
      int *updated = level_textures.chunks[index].updated;
      int count    = level_textures.chunks[index].count;

      level_textures.chunks[index].update = 0;

      SDL_SetRenderTarget(renderer, level_textures.chunks[index].tex);
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

      if (!count) {
        level_textures.chunks[index].tile_count = 0;
        SDL_RenderClear(renderer);
      }

      SDL_Rect ra, rb;
      int w  = level_width;
      int h  = level_height;
      int tw = tiles_tex_width  / rtile_width;
      int fromx = (ix*CHUNK_WIDTH) / tile_width;
      int fromy = (iy*CHUNK_HEIGHT) / tile_height;
      int tox = fromx + (CHUNK_WIDTH / tile_width);
      int toy = fromy + (CHUNK_HEIGHT / tile_height);
      for (int y=fromy; y<toy; y++) {
        for (int x=fromx; x<tox; x++) {
          // current tile
          int tindex = (y*w)+x;
          int tile = 0;
          if (x >= 0 && x < w && y >= 0 && y < h)
            tile = level.layers[level.layer].tiles[tindex];

          if (!tile)
            continue;

          // check if anything needs updating
          if (count) {
            int found = 0;
            for (int i=0; i<count; i++) {
              if (tindex == updated[i]) {
                found = 1;
                break;
              }
            }

            // no need to update it
            if (!found)
              continue;
          }

          // position of tile in texture
          ra.x = (tile - ((tile / tw) * tw)) * rtile_width;
          ra.y = (tile / tw) * rtile_height;
          ra.w = rtile_width;
          ra.h = rtile_height;

          // position of tile in map
          rb.x = (x - fromx) * tile_width;
          rb.y = (y - fromy) * tile_height;
          rb.w = tile_width;
          rb.h = tile_height;

          SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);

          if (!count)
            level_textures.chunks[index].tile_count++;
        }
      }

      level_textures.chunks[index].count = 0;
    }
  }
  /*----------------------------------------*/



  /*-----------------------------------------/
  /---------------- CHUNKS TO TEXTURE -------/
  /-----------------------------------------*/
  SDL_SetRenderTarget(renderer, tex_map);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  SDL_Rect r;
  cx = (camx + tile_width / 2) - (window_width / 2);
  cy = (camy + tile_height / 2) - (game_height / 2);
  int fromx = floor(cx / (float)CHUNK_WIDTH);
  int fromy = floor(cy / (float)CHUNK_HEIGHT);
  int tox = fromx + ceil((float)(window_width + CHUNK_WIDTH) / (float)CHUNK_WIDTH);
  int toy = fromy + ceil((float)(game_height + CHUNK_HEIGHT) / (float)CHUNK_HEIGHT);
  for (int y=fromy; y<toy; y++) {
    for (int x=fromx; x<tox; x++) {
      if (x < 0 || y < 0 || x >= CHUNK_STRIDE || y >= CHUNK_STRIDE)
        continue;

      int index = (y*CHUNK_STRIDE)+x;

      if (!level_textures.chunks[index].tile_count)
        continue;

      r.x = (int)floor(((x * CHUNK_WIDTH) - (cx)));
      r.y = (int)floor(((y * CHUNK_HEIGHT) - (cy)));
      r.w = (int)CHUNK_WIDTH;
      r.h = (int)CHUNK_HEIGHT;

      SDL_RenderCopy(renderer, level_textures.chunks[index].tex, NULL, &r);
    }
  }
  /*----------------------------------------*/



  /*-----------------------------------------/
  /---------------- FOV LIGHT MAP -----------/
  /-----------------------------------------*/
  SDL_SetRenderTarget(renderer, tex_fov);

  uint8_t *pixels = NULL;
  int pitch = 0;
  SDL_LockTexture(tex_fov, NULL, (void**)&pixels, &pitch);

  fromx = floor(cx / tile_width);
  fromy = floor(cy / tile_height);
  tox = MAX(0, MIN(fromx + (window_width / tile_width), level_width));
  toy = MAX(0, MIN(fromy + (game_height / tile_height) + 2, level_height));
  int count = ((tox - fromx) + 2) * 4;

  int errx = 0;
  if (fromx < 0)
    errx = abs(fromx);
  if (fromx > level_width)
    errx = (fromx - level_width);
  fromx += errx;
  tox -= errx;
  count -= errx * 4;
  if (tox >= level_width)
    count -= 8 + (tox - level_width) * 4;

  for (int y=fromy; y<toy; y++) {
    if (y < 0 || y > level_height)
      continue;
    int index = ((y*level_width)+fromx)*4;
    memcpy(&pixels[((y-fromy)*pitch)+(errx*4)], &light_map[index], count);
  }

  SDL_UnlockTexture(tex_fov);
  /*----------------------------------------*/



  /*-----------------------------------------/
  /---------------- RENDER ------------------/
  /-----------------------------------------*/
  // render to default target
  SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderClear(renderer);

  // render 1
  if (menu) {
    if (!menuinout)
      menualpha += 0.4f * delta_time;
    if (!menuinout && menualpha >= 1.0f) {
      menualpha = 4.0f;
      menuinout = 1;
    }
    if (menuinout && menualpha > 0)
      menualpha -= 0.4f * delta_time;
    if (menuinout && menualpha <= 0.0f) {
      menu = 0;
    }

    SDL_SetTextureAlphaMod(tex_menu, MAX(0, MIN(255, (int)(255.0f * menualpha))));
    SDL_RenderCopy(renderer, tex_menu, NULL, NULL);
    SDL_RenderPresent(renderer);
    return;
  }

  // render the level
  r.x = 0, r.y = 0, r.w = window_width, r.h = game_height;
  SDL_RenderCopy(renderer, tex_map, NULL, &r);

  spell_render_fire();

  item_render();

  entity_render();

  spell_render();

  // FoV/lighting
  float rcx = (cx / tile_width);
  float rcy = (cy / tile_height);
  r.x = ((floor(rcx) - rcx) * tile_width) - 1;
  r.y = ((floor(rcy) - rcy) * tile_height) - 1;
  r.w = ((window_width / tile_width) + 2) * tile_width;
  r.h = ((game_height / tile_height) + 2) * tile_height;
  SDL_RenderCopy(renderer, tex_fov, NULL, &r);

  // player ui
  // max hp
  char buff[512];
  for (int i=0; i<player->hp_max; i++)
    buff[i] = (char)3;
  buff[player->hp_max] = '\0';
  SDL_SetTextureColorMod(tex_font, 255, 255, 255);
  SDL_SetTextureAlphaMod(tex_font, 100);
  text_render(buff, 16, 16);

  // current hp
  for (int i=0; i<player->hp; i++)
    buff[i] = (char)3;
  buff[player->hp] = '\0';
  SDL_SetTextureColorMod(tex_font, 255, 46, 136);
  SDL_SetTextureAlphaMod(tex_font, 255);
  text_render(buff, 16, 16);

  if (experience > player->hp_max / 2) {
    player->hp_max++;
    player->hp = player->hp_max;
    plevel++;
    experience = 0;
    text_log_add("You feel stronger");
  }

  // max exp
  for (int i=0; i<player->hp_max / 2; i++)
    buff[i] = (char)16;
  buff[player->hp_max / 2] = '\0';
  SDL_SetTextureColorMod(tex_font, 255, 255, 255);
  SDL_SetTextureAlphaMod(tex_font, 100);
  text_render(buff, 16, 32);

  // current exp
  for (int i=0; i<experience; i++)
    buff[i] = (char)16;
  buff[experience] = '\0';
  SDL_SetTextureColorMod(tex_font, 104, 174, 255);
  SDL_SetTextureAlphaMod(tex_font, 255);
  text_render(buff, 16, 32);

  // inventory
  int y = 64;
  SDL_SetTextureColorMod(tex_font, 255, 255, 255);
  SDL_SetTextureAlphaMod(tex_font, 255);
  sprintf(buff, "%c backpack %c", 4, 4);
  text_render(buff, 16, y);
  for (int i=0; i<INVENTORY_MAX; i++) {
    y += 16;
    if (inventory[i]) {
      if (uses[i])
        sprintf(buff, "[%i] %c %s (%i)\n", i+1, 26, item_string[inventory[i]], uses[i]);
      else
        sprintf(buff, "[%i] %c %s (inf)\n", i+1, 26, item_string[inventory[i]], uses[i]);
    } else {
      sprintf(buff, "[%i] %c %s\n", i+1, 26, item_string[inventory[i]], uses[i]);
    }
    text_render(buff, 16, y);
  }
  y+=32;
  sprintf(buff, "%c actions %c", 4, 4);
  text_render(buff, 16, y);
  y+=16;
  sprintf(buff, "[c] %c %s\n", 26, "toggle door");
  text_render(buff, 16, y);
  y+=16;
  sprintf(buff, "[d] %c %s\n", 26, "drop item");
  text_render(buff, 16, y);
  y+=16;
  sprintf(buff, "[f] %c %s\n", 26, "fall down");
  text_render(buff, 16, y);
  y+=16;
  sprintf(buff, "[space] %c %s\n", 26, "interact");
  text_render(buff, 16, y);
  y+=16;

  player_render();

  // render text box
  text_log_render();

  // handle reset
  if (!player->alive) {
    text_render("press R to restart", 16, 296);
    if (keys_down[SDL_SCANCODE_R]) {
      init = 0;
    }
  }

  SDL_RenderPresent(renderer);
  /*----------------------------------------*/
}

void keypressed(int key)
{
  if (menu) {
    menuinout = 1;
    menualpha = MIN(1.0f, menualpha);
    return;
  }

  player_keypress(key);

  if (key == SDL_SCANCODE_SPACE && check_tile(player->to.x, player->to.y) == TILE_EXIT) {
    text_log_add("The stairs crumble behind you, sealing your fate");
    godown();
  }
}

void mousepress(int key)
{
  SDL_SetWindowInputFocus(window);
  SDL_SetWindowGrab(window, SDL_TRUE);

  if (menu)
    return;

  int tmx = ((mx + cx)) / tile_width;
  int tmy = ((my + cy)) / tile_height;
  tmx = MAX(0, MIN(tmx, level_width));
  tmy = MAX(0, MIN(tmy, level_height));

  if (tmx == player->to.x && tmy == player->to.y && check_tile(player->to.x, player->to.y) == TILE_EXIT) {
    text_log_add("The stairs crumble behind you, sealing your fate");
    godown();
  }

  player_mousepress(key, mx, my);
}

void input(SDL_Event *event)
{
  switch (event->type) {
    case SDL_KEYDOWN: {
      int key = event->key.keysym.scancode;
      keys_down[key] = 1;
      if (event->key.repeat == 0)
        keypressed(key);
      break;
    }
    case SDL_KEYUP: {
      int key = event->key.keysym.scancode;
      keys_down[key] = 0;
      break;
    }
    case SDL_MOUSEBUTTONDOWN: {
      mousepress(event->button.button);
    }
  }
}

int main(int argc, char **argv)
{
  // set random seed
  unsigned long seed = time(NULL);
  srand(seed);
  printf("SEED: %lu\n\n", seed);

  // start main loop, this doesnt ever return
  emscripten_set_main_loop(loop, 0, 1);

  // bye
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 1;
}