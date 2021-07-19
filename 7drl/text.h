#ifndef TEXT_H
#define TEXT_H

#include <SDL2/SDL.h>

extern SDL_Texture *tex_font, *tex_ui, *tex_text;

void text_init();

void text_log_add(const char *str);

void text_log_render();

void text_render(const char *str, int x, int y);

#endif // TEXT_H