#include "player.h"
#include "main.h"
#include "generator.h"
#include "spell.h"
#include "text.h"
#include "item.h"

entity_t *player = NULL;
int *dmap_to_player = NULL, *dmap_from_player = NULL;
int *dmap_to_mouse = NULL;
uint8_t *light_map = NULL;

int inventory[INVENTORY_MAX] = {0};
int uses[INVENTORY_MAX] = {0};
int active_slot = -1;

int speed_boost = 0;
int heal_rate = 0;

int experience = 0;
int plevel = 0;
int pkills = 0;

int aiming = 0, flame = 1;
int active_spell = -1;
ivec2_t fire_to;

int last_move_x = -DIJ_MAX, last_move_y = -DIJ_MAX;
int path_x, path_y;

int inventory_add(int item, int use)
{
  update = 1;

  for (int i=0; i<INVENTORY_MAX; i++) {
    if (!inventory[i]) {
      inventory[i] = item;
      uses[i] = use;
      return 1;
    }
  }

  return 0;
}

void inventory_remove(int index)
{
  update = 1;

  if (inventory[index]) {
    if (inventory[index] == ITEM_FIRETORCH)
      text_log_add("You drop your firetorch, the room darkens");
    item_new(player->to.x, player->to.y, inventory[index], uses[index]);
    inventory[index] = 0;
    uses[index] = 0;
  }
}

void inventory_use(int index)
{
  update = 1;
  active_slot = index;

  switch (inventory[index]) {
    case ITEM_FIRETORCH: {
      text_log_add("Your torch is already lit");
      break;
    }
    case ITEM_FIREBOLT: {
      active_spell = SPELL_FIREBOLT;
      text_log_add("You ready a firebolt");
      aiming = 1;
      break;
    }
    case ITEM_FIRESURGE: {
      active_spell = SPELL_FIRESURGE;
      text_log_add("You ready a firesurge");
      aiming = 1;
      break;
    }
    case ITEM_FIRESTORM: {
      active_spell = SPELL_FIRESTORM;
      text_log_add("You ready a firestorm");
      aiming = 1;
      break;
    }
    case ITEM_FIRESPRAY: {
      active_spell = SPELL_FIRESPRAY;
      text_log_add("You ready a firespray");
      aiming = 1;
      break;
    }
    case ITEM_FIREPUSH: {
      active_spell = SPELL_FIREPUSH;
      text_log_add("You ready a firepush");
      aiming = 1;
      break;
    }
    case ITEM_FIREJUMP: {
      active_spell = SPELL_FIREJUMP;
      text_log_add("You ready a firejump");
      aiming = 1;
      break;
    }
    case ITEM_SPIRIT: {
      active_spell = SPELL_SPIRIT;
      text_log_add("You ready a spirit orb");
      aiming = 1;
      break;
    }
    case ITEM_FOOD: {
      player->hp = MIN(player->hp + 2 + roll(2) + plevel, player->hp_max);
      text_log_add("You consume some of the cooked flesh and recover some health");
      uses[index]--;
      if (uses[index] <= 0)
        inventory[index] = 0;
      break;
    }
    case ITEM_FLESH: {
      player->hp = MIN(player->hp + 1, player->hp_max);
      text_log_add("You consume the raw flesh and recover a small amount of health");
      inventory[index] = 0;
      break;
    }
    case ITEM_POT: {
      text_log_add("You consume the potion");
      text_log_add("You feel faster");
      speed_boost = 5 + roll(3);
      inventory[index] = 0;
      break;
    }
  }
}

void player_init(int x, int y)
{
  // discard if already initialized
  if (player)
    player->alive = 0;

  // initialize an entity for the player
  player = entity_new();
  player->onhit  = player_hit;
  player->ondeath= player_die;
  player->update = NULL;
  player->alive  = 1;
  player->hp     = 10;
  player->hp_max = player->hp;
  player->speed  = 2;
  player->pos.x  = x;
  player->pos.y  = y;
  player->to.x   = x;
  player->to.y   = y;
  fire_to.x      = x;
  fire_to.y      = y;
  aiming         = 0;

  for (int i=0; i<INVENTORY_MAX; i++)
    inventory[i] = 0;

  inventory[0] = ITEM_FIRETORCH;
  inventory[1] = ITEM_FIREBOLT;
  uses[1] = item_uses[ITEM_FIREBOLT];
  inventory[2] = ITEM_FOOD;
  uses[2] = item_uses[ITEM_FOOD];

  // tile 16
  entity_set_tile(player, 0, TILEI_PLAYER);

  player_reinit();

  plevel = 0;
  pkills = 0;
  experience = 0;
}

void player_reinit()
{
  // initialize dmap arrays
  if (dmap_to_mouse)
    free(dmap_to_mouse);
  dmap_to_mouse = malloc(sizeof(int) * level_width * level_height);
  if (dmap_to_player)
    free(dmap_to_player);
  dmap_to_player = malloc(sizeof(int) * level_width * level_height);
  if (dmap_from_player)
    free(dmap_from_player);
  dmap_from_player = malloc(sizeof(int) * level_width * level_height);

  if (light_map)
    free(light_map);
  light_map = malloc(level_width * level_height * 4);

  player_update(player);
  player_light();

  aiming = 0;
}

void player_die(entity_t *e)
{
  char buff[256];
  sprintf(buff, "You have perished on floor %i", layer);
  text_log_add(buff);
  sprintf(buff, "You where level %i", plevel);
  text_log_add(buff);
  sprintf(buff, "You have slain %i", pkills);
  text_log_add(buff);
}

void player_hit(entity_t *e, int damage, int type)
{
  if (damage > 0) {
    char buff[512];
    sprintf(buff, "You take %i damage", damage);
    text_log_add(buff);
    heal_rate = HEALRATE;
  }

  if (type == SPELL_WEB) {
    text_log_add("You get caught in a spider web");
    text_log_add("You cannot move");
    player->stuck = 1 + roll(3);
  }
}

void player_light()
{
  /*-----------------------------------------/
  /---------------- FOV ---------------------/
  /-----------------------------------------*/
  for (int i=0; i<level_width*level_height; i++) {
    light_map[(i*4)+0] = 0;
    light_map[(i*4)+1] = 0;
    light_map[(i*4)+2] = 0;
    light_map[(i*4)+3] = 255;
  }

  char *tiles = level.layers[level.layer].tiles;

  ivec2_t positions[512] = {0};

  int walls[32] = {-1};
    for (int i=0; i<SOLID_COUNT; i++)
      walls[i] = solid[i];
  walls[SOLID_COUNT-1] = TILE_DOOR_CLOSED;

  int distance = flame ? 50 : 16;
  distance = MAX(4, distance - (layer * 2));

  for (int i=0; i<360; i++) {
    float fromx = player->to.x;
    float fromy = player->to.y;

    float tox = fromx + 0.5f + ((float)distance * sintable[i]);
    float toy = fromy + 0.5f + ((float)distance * costable[i]);

    int count = line(fromx, fromy, tox, toy, level_width, level_height, tiles, walls, positions);

    for (int j=0; j<count; j++) {
      // get position and light value
      int x = MAX(0, MIN(positions[j].x, level_width));
      int y = MAX(0, MIN(positions[j].y, level_height));
      int val = 255 - MAX(0, 255 - MIN(((float)j / (float)distance)*255, 255));
      if (count < distance)
        val = 255 - MAX(0, 255 - MIN(((float)j / (float)count)*255, 255));

      // set pixeldata
      int index = ((y*level_width)+x)*4;
      if (light_map[index+3] > val) {
        light_map[index+0] = 0;
        light_map[index+1] = 0;
        light_map[index+2] = 0;
        light_map[index+3] = val;
      }

      int tile = check_tile(x, y);
      if (tile == TILE_STONE_HWALL || tile == TILE_STONE_VWALL || tile == TILE_DOOR_CLOSED || tile == TILE_DOOR_OPEN)
        continue;

      // light dark walls that are next to lit floor
      for (int k=0; k<8; k++) {
        int cx = MAX(0, MIN(x + around[k][0], level_width));
        int cy = MAX(0, MIN(y + around[k][1], level_height));
        if (cx == x && cy == y)
          continue;

        int tile = check_tile(cx, cy);
        int index = ((cy*level_width)+cx)*4;
        if (light_map[index+3] > val) {
          light_map[index+0] = 0;
          light_map[index+1] = 0;
          light_map[index+2] = 0;
          light_map[index+3] = val;
        }
      }
    }
  }
  /*----------------------------------------*/
}

void player_fire()
{
  if (!player->alive)
    return;

  aiming = 0;
  update = 1;

  int slot = active_slot;
  if (active_slot > -1) {
    uses[active_slot]--;
    if (uses[active_slot] <= 0) {
      inventory[active_slot] = 0;
      uses[active_slot] = 0;
    }
    active_slot = -1;
  }

  char buff[256];
  switch (active_spell) {
    case SPELL_FIREBOLT: {
      text_log_add("You cast a fireball");
      sprintf(buff, "The remaining number of uses are %i", uses[slot]);
      text_log_add(buff);
      break;
    }
    case SPELL_FIRESURGE: {
      text_log_add("You cast a surge of fireballs");
      sprintf(buff, "The remaining number of uses are %i", uses[slot]);
      text_log_add(buff);
      break;
    }
    case SPELL_FIRESTORM: {
      text_log_add("You cast a firestorm");
      sprintf(buff, "The remaining number of uses are %i", uses[slot]);
      text_log_add(buff);
      break;
    }
    case SPELL_FIRESPRAY: {
      text_log_add("You cast firespray");
      sprintf(buff, "The remaining number of uses are %i", uses[slot]);
      text_log_add(buff);
      break;
    }
    case SPELL_FIREPUSH: {
      if (player->to.x == fire_to.x && player->to.y == fire_to.y) {
        update = 0;
        return;
      }
      text_log_add("You cast firepush");
      int dx = fire_to.x - player->to.x;
      int dy = fire_to.y - player->to.y;
      entity_push(player->to.x+dx, player->to.y+dy, dx*20, dy*20);
      entity_push(player->to.x, player->to.y, -dx*2, -dy*2);
      sprintf(buff, "The remaining number of uses are %i", uses[slot]);
      text_log_add(buff);
      return;
    }
    case SPELL_FIREJUMP: {
      if (player->to.x == fire_to.x && player->to.y == fire_to.y) {
        update = 0;
        return;
      }
      text_log_add("You cast firejump");
      player->to.x = fire_to.x;
      player->to.y = fire_to.y;
      sprintf(buff, "The remaining number of uses are %i", uses[slot]);
      text_log_add(buff);
      return;
    }
    case SPELL_SPIRIT: {
      text_log_add("You cast spirit orb");
      sprintf(buff, "The remaining number of uses are %i", uses[slot]);
      text_log_add(buff);
      break;
    }
    case CLOSE_DOOR: {
      if (player->to.x == fire_to.x && player->to.y == fire_to.y) {
        update = 0;
        return;
      }

      int tile = check_tile(fire_to.x, fire_to.y);
      if (tile == TILE_DOOR_CLOSED) {
        update_chunk(fire_to.x*tile_width, fire_to.y*tile_height, TILE_DOOR_OPEN);
        text_log_add("The door opens");
      } else if (tile == TILE_DOOR_OPEN) {
        update_chunk(fire_to.x*tile_width, fire_to.y*tile_height, TILE_DOOR_CLOSED);
        text_log_add("The door closes");
      }
      return;
    }
    case DROP_ITEM: {
      return;
    }
    case FALL_DOWN: {
      int tile = check_tile(fire_to.x, fire_to.y);
      if (tile == TILE_WOOD_HOLE) {
        if (layer == MAX_LAYER) {
          text_log_add("You refuse to jump into the abyss below");
        } else {
          player->hp -= (player->hp_max / 2);
          if (player->hp > 0) {
            text_log_add("You fall down and take some damage");
            godown();
          } else {
            text_log_add("You fall to your death");
            player_die(player);
            player->alive = 0;
          }
        }
      }
      return;
    }
  }

  spell_new(active_spell, player->to.x, player->to.y, fire_to.x, fire_to.y);
}

int player_update()
{
  int update = 0;

  if (!player->alive) {
    player->hp = 0;
    return 1;
  }

  player->speed = 2;
  if (speed_boost) {
    speed_boost--;
    player->speed = 4;
  }

  heal_rate--;
  if (heal_rate <= 0) {
    player->hp = MIN(player->hp+1, player->hp_max);
    heal_rate = HEALRATE;
  }

  flame = 0;
  for (int i=0; i<INVENTORY_MAX; i++)
    if (inventory[i] == ITEM_FIRETORCH)
      flame = 1;

  if (player->walking)
    update = 1;

  // regenerate dmap to player
  char *tiles = level.layers[level.layer].tiles;
  for (int i=0; i<level_width*level_height; i++)
    dmap_to_player[i] = DIJ_MAX;

  dmap_to_player[((int)player->to.y*level_width)+(int)player->to.x] = 0;

  int walls[32] = {-1};
  for (int i=0; i<SOLID_COUNT; i++)
    walls[i] = solid[i];

  int w = (window_width/tile_width)/2;
  int h = (window_height/tile_height)/2;
  int fromx = player->to.x - w;
  int fromy = player->to.y - h;
  int tox   = player->to.x + w;
  int toy   = player->to.y + h;
  dijkstra(dmap_to_player, tiles, walls, fromx, fromy, tox, toy, level_width, level_height);

  // calculate fleeing dmap map
  for (int i=0; i<level_width*level_height; i++) {
    if (dmap_to_player[i] == DIJ_MAX)
      dmap_from_player[i] = -DIJ_MAX;
    else
      dmap_from_player[i] = -(dmap_to_player[i] * 1.2f);
  }

  // recalculate it
  dijkstra(dmap_from_player, tiles, walls, fromx, fromy, tox, toy, level_width, level_height);

  return update;
}

void player_render()
{
  if (!player->alive) {
    update = 1;
    return;
  }

  if (!aiming) {
    active_slot = -1;
    active_spell = -1;
  }

  SDL_Rect ra, rb;
  if (player->stuck) {
    ivec2_t t;
    int tw = tiles_tex_width  / rtile_width;
    picktile(&t, TILE_SPELL_WEB, tw, rtile_width, rtile_height);

    ra.x = t.x;
    ra.y = t.y;
    ra.w = tile_width;
    ra.h = tile_height;
    rb.w = tile_width;
    rb.h = tile_height;
    rb.x = (player->pos.x*tile_width)-cx;
    rb.y = (player->pos.y*tile_height)-cy;
    SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);
  }

  /*-----------------------------------------/
  /---------------- AIMING ------------------/
  /-----------------------------------------*/
  int tmx = ((mx + cx)) / tile_width;
  int tmy = ((my + cy)) / tile_height;
  tmx = MAX(0, MIN(tmx, level_width));
  tmy = MAX(0, MIN(tmy, level_height));
  SDL_SetTextureColorMod(tex_tiles, 255, 255, 0);
  if (aiming && active_spell > -1) {
    // collidable tiles
    int walls[32] = {-1};
      for (int i=0; i<SOLID_COUNT; i++)
        walls[i] = solid[i];
    walls[SOLID_COUNT-1] = TILE_DOOR_CLOSED;

    // raycast
    char *tiles = level.layers[level.layer].tiles;
    ivec2_t positions[512] = {0};

    int count = line(player->to.x, player->to.y, tmx, tmy, level_width, level_height, tiles, walls, positions);

    ra.x = 0;
    ra.y = 0;
    ra.w = rtile_width;
    ra.h = rtile_height;
    rb.w = tile_width;
    rb.h = tile_height;
    fire_to.x = player->to.x;
    fire_to.y = player->to.y;
    for (int j=1; j<count; j++) {
      int tile = check_tile(positions[j].x, positions[j].y);
      if (j > spell_get_range(active_spell) || (tile == TILE_STONE_HWALL || tile == TILE_STONE_VWALL))
        break;
      rb.x = MAX(0, MIN(positions[j].x, level_width)) * tile_width;
      rb.y = MAX(0, MIN(positions[j].y, level_height)) * tile_height;
      rb.x -= cx;
      rb.y -= cy;
      fire_to.x = positions[j].x;
      fire_to.y = positions[j].y;
      SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);
    }
  }
  SDL_SetTextureColorMod(tex_tiles, 255, 255, 255);
  /*----------------------------------------*/

  /*-----------------------------------------/
  /---------------- DMAP TO MOUSE -----------/
  /-----------------------------------------*/
  if ((last_move_x != tmx || last_move_y != tmy) && !player->walking) {
    last_move_x = tmx;
    last_move_y = tmy;

    int tile = level.layers[level.layer].tiles[(tmy*level_width)+tmx];
    if (!check_solid(tile)) {
      // regenerate dmap to mouse
      char *tiles = level.layers[level.layer].tiles;
      for (int i=0; i<level_width*level_height; i++)
        dmap_to_mouse[i] = DIJ_MAX;

      dmap_to_mouse[(tmy*level_width)+tmx] = 0;

      int walls2[32] = {-1};
      for (int i=0; i<SOLID_COUNT; i++)
        walls2[i] = solid[i];

      int w = (window_width/tile_width)/2;
      int h = (window_height/tile_height)/2;
      int fromx = player->to.x - w;
      int fromy = player->to.y - h;
      int tox   = player->to.x + w;
      int toy   = player->to.y + h;
      dijkstra(dmap_to_mouse, tiles, walls2, fromx, fromy, tox, toy, level_width, level_height);
    }
  }

  player->dmap = dmap_to_mouse;

  if (aiming) {
    switch (active_spell) {
      case SPELL_FIREBOLT: {
        text_render("casting firebolt [dir]", 16, 296);
        break;
      }
      case SPELL_FIRESURGE: {
        text_render("casting firesurge [dir]", 16, 296);
        break;
      }
      case SPELL_FIRESTORM: {
        text_render("casting firestorm [dir]", 16, 296);
        break;
      }
      case SPELL_FIRESPRAY: {
        text_render("casting firespray [dir]", 16, 296);
        break;
      }
      case SPELL_FIREPUSH: {
        text_render("casting firepush [dir]", 16, 296);
        break;
      }
      case SPELL_FIREJUMP: {
        text_render("casting firejump [dir]", 16, 296);
        break;
      }
      case SPELL_WEB: {
        text_render("casting web [dir]", 16, 296);
        break;
      }
      case SPELL_SPIRIT: {
        text_render("casting spirit orb [dir]", 16, 296);
        break;
      }
      case CLOSE_DOOR: {
        text_render("toggle door [dir]", 16, 296);
        break;
      }
      case DROP_ITEM: {
        text_render("drop item [1-5]", 16, 296);
        break;
      }
      case FALL_DOWN: {
        text_render("fall down hole [dir]", 16, 296);
        break;
      }
    }
  }

  if (aiming)
    return;

  ra.x = 0;
  ra.y = 0;
  ra.w = rtile_width;
  ra.h = rtile_height;
  rb.w = tile_width;
  rb.h = tile_height;

  int ex = player->to.x;
  int ey = player->to.y;
  int i;
  for (i=0; i<20; i++) {
    int found_tile = 0;
    int findx = ex, findy = ey;
    int initial = player->dmap[(ey*level_width)+ex];
    int lowest = initial;

    if (!lowest)
      break;

    if (!((i/player->speed) % 2))
      SDL_SetTextureColorMod(tex_tiles, 255, 46, 136);
    else
      SDL_SetTextureColorMod(tex_tiles, 46, 255, 136);

    SDL_SetTextureAlphaMod(tex_tiles, 255 - ((i / 10.0f) * 255.0f));

    for (int j=0; j<8; j++) {
      int tx = MAX(0, MIN(ex + around_adjacent[j][0], level_width));
      int ty = MAX(0, MIN(ey + around_adjacent[j][1], level_height));
      int tile = level.layers[level.layer].tiles[(ty*level_width)+tx];

      if (tx == ex && ty == ey)
        continue;

      if (check_solid(tile))
        continue;

      int value = player->dmap[(ty*level_width)+tx];

      // is it the lowest value tile?
      if (value < lowest) {
        lowest = value;
        findx  = tx;
        findy  = ty;
        found_tile = tile;
      }
    }

    if (lowest == initial)
      break;

    if (lowest != DIJ_MAX) {
      ex = findx;
      ey = findy;
      path_x = findx;
      path_y = findy;
      rb.x = (findx * tile_width) - cx;
      rb.y = (findy * tile_height) - cy;

      SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);
    } else {
      break;
    }
  }
  SDL_SetTextureColorMod(tex_tiles, 255, 255, 255);
  SDL_SetTextureAlphaMod(tex_tiles, 255);

  int turns = ((MAX(i,1)-1)/player->speed)+1;
  char buff[512];
  if (turns == 1)
    sprintf(buff, "%i turn", turns);
  else
    sprintf(buff, "%i turns", turns);
  text_render(buff, 16, 296);
  /*----------------------------------------*/
}

void player_keypress(int key)
{
  if (!player->alive) {
    update = 1;
    return;
  }

  if (player->walking) {
    player->walking = 0;
    return;
  }

  if (key == SDL_SCANCODE_SPACE) {
    item_t *item = item_take(player->to.x, player->to.y);

    if (item) {
      if (!inventory_add(item->item, item->uses)) {
        item_new(player->to.x, player->to.y, item->item, item->uses);
      }
    } else {
      text_log_add("You rest for a turn");
    }

    update = 1;
  }

  if (active_spell == DROP_ITEM) {
    if (key == SDL_SCANCODE_1)
      inventory_remove(0);
    if (key == SDL_SCANCODE_2)
      inventory_remove(1);
    if (key == SDL_SCANCODE_3)
      inventory_remove(2);
    if (key == SDL_SCANCODE_4)
      inventory_remove(3);
    if (key == SDL_SCANCODE_5)
      inventory_remove(4);
    if (key == SDL_SCANCODE_6)
      inventory_remove(5);

    active_spell = -1;
    aiming = 0;
  }

  if (aiming) {
    text_log_add("You stop casting");
  }

  active_spell = -1;
  aiming = 0;

  if (active_spell < 0) {
    if (key == SDL_SCANCODE_1)
      inventory_use(0);
    if (key == SDL_SCANCODE_2)
      inventory_use(1);
    if (key == SDL_SCANCODE_3)
      inventory_use(2);
    if (key == SDL_SCANCODE_4)
      inventory_use(3);
    if (key == SDL_SCANCODE_5)
      inventory_use(4);
    if (key == SDL_SCANCODE_6)
      inventory_use(5);
  }

  if (key == SDL_SCANCODE_C) {
    active_spell = CLOSE_DOOR;
    aiming = 1;
  }
  if (key == SDL_SCANCODE_D) {
    active_spell = DROP_ITEM;
    aiming = 1;
  }
  if (key == SDL_SCANCODE_F) {
    active_spell = FALL_DOWN;
    aiming = 1;
  }
}

void player_mousepress(int button, int mx, int my)
{
  if (!player->alive) {
    update = 1;
    return;
  }
  if (aiming && active_spell > -1) {
    player_fire();
    return;
  }

  /*-----------------------------------------/
  /---------------- mouse dmap --------------/
  /-----------------------------------------*/
  int tmx = ((mx + cx)) / tile_width;
  int tmy = ((my + cy)) / tile_height;
  tmx = MAX(0, MIN(tmx, level_width));
  tmy = MAX(0, MIN(tmy, level_height));

  if (!update && tmx == player->to.x && tmy == player->to.y) {
    item_t *item = item_take(player->to.x, player->to.y);

    if (item) {
      if (!inventory_add(item->item, item->uses)) {
        item_new(player->to.x, player->to.y, item->item, item->uses);
      }
    } else {
      text_log_add("You rest for a turn");
    }

    update = 1;
    return;
  }

  if (player->walking) {
    player->walking = 0;
    return;
  }

  int tile = level.layers[level.layer].tiles[(path_y*level_width)+path_x];
  if (check_solid(tile))
    return;

  player->walking = 1;

  // run an update pass
  update = 1;
  tick = -(1.0 / 16.0);
  /*----------------------------------------*/
}