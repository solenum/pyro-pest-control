#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TEXTURE_LOC "data/textures/"

SDL_Texture* texture_load(const char *file_name, int *width, int *height)
{
  // prepend file directory
  size_t len = strlen(TEXTURE_LOC);
  char file_dir[len + strlen(file_name)];
  strcpy(file_dir, TEXTURE_LOC);
  strcpy(&file_dir[len], file_name);

  printf("Loading texture %s\n", file_dir);

  // attempt to load image
  int w,h,n;
  uint8_t *data = stbi_load(file_dir, &w, &h, &n, 4);
  if (data == NULL) {
    printf("Could not load texture %s\n", file_dir);
    return NULL;
  }

  // create a texture
  SDL_Texture *tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, w, h);

  // update it with the pixeldata
  SDL_UpdateTexture(tex, NULL, data, w*n);

  if (width)
    *width = w;
  if (height)
    *height = h;

  return tex;
}