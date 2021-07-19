#ifndef TEXTURE_H
#define TEXTURE_H

#include "main.h"

SDL_Texture* texture_load(const char *file_name, int *width, int *height);

#endif // TEXTURE_H