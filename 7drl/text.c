#include "main.h"
#include "text.h"
#include "math.h"
#include "texture.h"
#include "vga.h"
#include "entity.h"
#include "spell.h"
#include "player.h"

#include <inttypes.h>

SDL_Texture *tex_font = NULL, *tex_text = NULL;

#define LOG_MAX 7
#define TEXT_AREA_WIDTH  515
#define TEXT_AREA_HEIGHT 128

char text_log[LOG_MAX][512];
int text_logi = 0, text_update = 0;

void text_init()
{
  uint8_t fg[] = {255, 255, 255, 255};
  uint8_t bg[] = {0, 0, 255, 0};

  uint8_t *data = malloc(sizeof(uint8_t) * VGA_WIDTH * VGA_HEIGHT * 4);
  memset(data, 0, sizeof(uint8_t) * VGA_WIDTH * VGA_HEIGHT * 4);

  int x = 0, y = 0;
  for (int i=0; i<256; i++) {
    int index = i*VGA_FONT_HEIGHT;
    uint8_t *glyph = &vga_font_array[index];

    // make a glyph from vga bitmap
    uint8_t pixels[VGA_FONT_WIDTH*VGA_FONT_HEIGHT*4] = {0};
    for (int j=0; j<VGA_FONT_HEIGHT; j++) {
      uint8_t byte = glyph[j];

      for (int k=0; k<VGA_FONT_WIDTH; k++) {
        int color = (byte >> k) & 0x01;
        int pindex = ((j*VGA_FONT_WIDTH)+(VGA_FONT_WIDTH-k))*4;
        if (color)
          memcpy(&pixels[pindex], fg, sizeof(uint8_t) * 4);
        else
          memcpy(&pixels[pindex], bg, sizeof(uint8_t) * 4);
      }
    }

    // copy glyph pixels to atlas
    for (int iy=0; iy<VGA_FONT_HEIGHT; iy++) {
      for (int ix=0; ix<VGA_FONT_WIDTH; ix++) {
        int i1  = (((iy+y)*VGA_WIDTH)+x+ix)*4;
        int i2 = ((iy*VGA_FONT_WIDTH)+ix)*4;
        memcpy(&data[i1], &pixels[i2], sizeof(uint8_t) * 4);
      }
    }

    // advance to the next glyph position
    x += VGA_FONT_WIDTH;
    if (x >= VGA_WIDTH) {
      x = 0;
      y += VGA_FONT_HEIGHT;
      if (y >= VGA_HEIGHT)
        break;
    }
  }

  // generate our texture
  if (!tex_font)
    tex_font = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, VGA_WIDTH, VGA_HEIGHT);
  SDL_SetTextureBlendMode(tex_font, SDL_BLENDMODE_BLEND);

  // update it with the pixeldata
  SDL_UpdateTexture(tex_font, NULL, data, VGA_WIDTH * 4);

  // text is rendered to this every time its updated
  // this is then rendered to the screen
  if (!tex_text)
    tex_text = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, TEXT_AREA_WIDTH, TEXT_AREA_HEIGHT);
  SDL_SetTextureBlendMode(tex_text, SDL_BLENDMODE_BLEND);

  // set the log to an empty state
  memset(text_log, '\0', 512 * LOG_MAX);

  // initialize the text log texture target
  text_log_add("\0");

  // lore
  text_log_add("You descend into the dungeon in search of the rat king");
  text_log_add("Defeat him on level 10 and then make your escape");
  text_log_add("Prepare well, items are sparse deeper in the dungeon");

  free(data);
}

void text_log_add(const char *str)
{
  if (!player->alive)
    return;

  memset(&text_log[text_logi][0], ' ', 512);
  memcpy(&text_log[text_logi++][0], str, MIN(strlen(str), 512));
  if (text_logi >= LOG_MAX)
    text_logi = 0;

  text_update = 1;
}

void text_log_render()
{
  if (text_update && entity_ready() && spell_ready()) {
    SDL_SetRenderTarget(renderer, tex_text);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetTextureColorMod(tex_font, 255, 255, 255);

    // render log lines
    int ypos = 7;
    int alpha = 255;
    for (int y=0; y<LOG_MAX; y++) {
      int i = text_logi-y-1;
      if (i < 0)
        i += 7;

      char *str = &text_log[i][0];
      if (str[0] == '\0')
        continue;

      SDL_SetTextureAlphaMod(tex_font, 255 - ((float)ypos / 128.0f) * 255);
      text_render(str, 12, (TEXT_AREA_HEIGHT - VGA_FONT_HEIGHT) - ypos);
      ypos += VGA_FONT_HEIGHT;
    }

    text_update = 0;
  }

  SDL_Rect r;
  r.x = 0, r.y = game_height-128;
  r.h = 128, r.w = TEXT_AREA_WIDTH;
  SDL_RenderCopy(renderer, tex_text, NULL, &r);
}

void text_render(const char *str, int x, int y)
{
  SDL_Rect ra,rb;
  rb.x = x;
  rb.y = y;
  rb.w = VGA_FONT_WIDTH;
  rb.h = VGA_FONT_HEIGHT;
  for (int i=0; i<strlen(str); i++) {
    char c = str[i];

    if (c == ' ') {
      rb.x += rb.w;
      continue;
    }

    if (c == '\n') {
      rb.y += rb.h;
      rb.x  = x;
      continue;
    }

    ra.x = (c * VGA_FONT_WIDTH) % VGA_WIDTH;
    ra.y = ((c * VGA_FONT_WIDTH) / VGA_WIDTH) * VGA_FONT_HEIGHT;
    ra.w = VGA_FONT_WIDTH;
    ra.h = VGA_FONT_HEIGHT;

    SDL_RenderCopy(renderer, tex_font, &ra, &rb);

    rb.x += rb.w;
  }
}