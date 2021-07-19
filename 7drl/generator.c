#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "main.h"
#include "generator.h"
#include "math.h"

// CELL STUFF
/* ---------------- */
#define CELL_NONE     0
#define CELL_START    1
#define CELL_END      2
#define CELL_HALL     3
#define CELL_PREFAB   4
#define CELL_TREASURE 5
#define CELL_POWER    6
#define CELL_CAVE     7
#define CELL_FILL     8
#define CELL_EMPTY    9
#define CELL_ROOM     10
#define CELL_TYPES    11

// min value set should be 1
#define CHANCE_CELL_REMOVE 15  // lower is more : 30
#define CHANCE_CELL_PREFAB 50  // lower is more : 50
#define CHANCE_CELL_ROOM   5   // lower is more : 8
#define CHANCE_CELL_CAVE   100 // lower is more : 50

#define CELL_MAX 1000

typedef struct {
  int size, num;
  char cells[CELL_MAX][CELL_MAX];
} cells_t;

int fill_cells(int x, int y, int t, int max, cells_t *c, int count)
{
  if (x < 0 || y < 0 || x > max || y > max)
    return count;

  if (c->cells[y][x] != CELL_EMPTY && c->cells[y][x] != t) {
    c->cells[y][x] = t;
    count++;
  } else {
    return count;
  }

  count = fill_cells(x+1, y+0, t, max, c, count);
  count = fill_cells(x-1, y+0, t, max, c, count);
  count = fill_cells(x+0, y+1, t, max, c, count);
  count = fill_cells(x+0, y-1, t, max, c, count);

  return count;
}

int dig_cells(ivec2_t *l, int index, int x, int y, int len, int c, int s)
{
  int dirs[4][2] = {
    {1,   0}, // right
    {0,   1}, // down
    {-1,  0}, // left
    {0,  -1}  // up
  };
  int dir = rand() % 4;

  while (len > 0) {
    len--;

    if (index == s*s)
      return index;

    if (x > s || x < 0 || y > s || y < 0)
      return index;

    int i = 0;
    for (; i<index; i++)
      if (l[i].x == x && l[i].y == y)
        break;

    if (i == index) {
      l[index].x = x;
      l[index].y = y;
      index++;
    }

    if (c && roll(c) == 1) {
      dir = (roll(2) == 1) ? dir-1 : dir+1;
      dir = MAX(0, MIN(dir, 3));

      x += dirs[dir][0];
      y += dirs[dir][1];
    } else {
      x += dirs[dir][0];
      y += dirs[dir][1];
    }
  }

  return index;
}

void pick_cell(ivec2_t *l, cells_t *c, int len, int *x, int *y, int no[CELL_TYPES], int yes[CELL_TYPES], int around, int size)
{
  int found_x = -1, found_y = -1;
  float score = 0.0f;

  for (int index=0; index<len; index++) {
    int x = MAX(0, MIN(l[index].x, size));
    int y = MAX(0, MIN(l[index].y, size));
    int cell_a = c->cells[y][x];

    // make sure this cell is on the
    // list of allowed types to pick
    int i;
    for (i=0; i<=CELL_TYPES; i++)
      if (i == CELL_TYPES || cell_a == yes[i])
        break;

    // its not on the list of allowed types
    if (i == CELL_TYPES)
      continue;

    // num surrounding cells
    int count=0;
    for (i=0; i<4; i++) {
      int cx = MAX(0, MIN(x + adjacent[i][0], size));
      int cy = MAX(0, MIN(y + adjacent[i][1], size));
      if (cx == x && cy == y)
        continue;

      int cell = c->cells[cy][cx];

      for (int j=0; j<CELL_TYPES; j++) {
        if (cell == yes[j]) {
          count++;
          break;
        }
      }
    }

    // not enough surrounding cells
    if (count < around)
      continue;

    // accumulate the distance of all
    // cell types on the 'no' list
    float d = -(c->size * c->size);
    for (int indexb=0; indexb<len; indexb++) {
      int k = l[indexb].x;
      int j = l[indexb].y;
      int cell_b = c->cells[j][k];

      // calculate score
      for (i=0; i<CELL_TYPES; i++) {
        if (cell_b == no[i]) {
          float d2 = hypot(x - k, y - j);
          if (d2 < fabs(d)) {
            d = d2;
            break;
          }
        }
      }
    }

    // found a cell to set
    if (d > score) {
      score = d;
      found_x = x;
      found_y = y;
    }
  }

  // have we found a cell to use?
  if (found_x >= 0 && found_y >= 0) {
    *x = found_x;
    *y = found_y;
  }
}

int generate_cells(cells_t *c, int size, int tidyness)
{
  size = MAX(8, MIN(size, CELL_MAX-1));
  c->size = size;

  // initialize cells to empty
  for (int y=0; y<=size; y++)
    for (int x=0; x<=size; x++)
      c->cells[y][x] = CELL_EMPTY;

  int starti = (size / 4);
  int endi = (size/2);

  // pick a random starting point
  int start_x = starti + rand() % (starti+endi);
  int start_y = starti + rand() % (starti+endi);
  c->cells[start_y][start_x] = CELL_START;

  // angle from start to center
  float x = ((float)size / 2.0f) - (float)start_x;
  float y = ((float)size / 2.0f) - (float)start_y;
  float d = atan2(y, x);

  // calculate end point random distance from start point
  // using the angle from start to center
  // also setting a min distance
  float dist = (size / 2) + (float)(rand() % (size / 2));
  float dx = (float)start_x + (int)(dist * cos(d));
  float dy = (float)start_y + (int)(dist * sin(d));

  // clamp values
  dx = dx > size ? size : dx;
  dy = dy > size ? size : dy;
  dx = dx < 0 ? 0 : dx;
  dy = dy < 0 ? 0 : dy;

  // angle from start to end
  d += (roll(2) == 1) ? 90.0f : -90.0f;
  int angles[2][2] = {
    {(int)(2 * cos(d)), 0},
    {0, (int)(2 * sin(d))}
  };
  int angle = rand() % 2;
  if (!angles[angle][0] && !angles[angle][1])
    angle = !angle;

  int end_x = (int)dx, end_y = (int)dy;
  int ix = start_x, iy = start_y;

  c->cells[end_y][end_x] = CELL_END;

  // random starting direction
  int dir    = rand() % 4;
  int dirs[4][2] = {
    {1,   0}, // right
    {0,   1}, // down
    {-1,  0}, // left
    {0,  -1}  // up
  };

  // list of cells walked over
  int index = 0;
  ivec2_t *l = malloc(sizeof(ivec2_t) * size * size);

  // walk
  for (;;) {
    ix = MAX(0, MIN(ix + dirs[dir][0], size));
    iy = MAX(0, MIN(iy + dirs[dir][1], size));

    int ex = end_x > ix ? 0 : 2;
    int ey = end_y > iy ? 1 : 3;
    int d = rand() % 2;
    dir = d ? ex : ey;

    if (end_y == iy)
      dir = ex;
    if (end_x == ix)
      dir = ey;

    // store cell position
    // avoiding duplicate positions
    int i = 0;
    for (; i<index; i++)
      if (index > 0 && l[i].x == ix && l[i].y == iy)
        break;

    if (i == index || index == 0) {
      l[index].x = ix;
      l[index].y = iy;
      index++;
    }

    // spawn another digger
    // minimal distance of size
    // chance of tidyness to change direction
    // size + roll(size / 2) ?
    index = dig_cells(l, index, ix, iy, roll(size/2), tidyness, size);

    // hit the end, spawn a load of
    // diggers at random positions
    // change roll(..) == 1 for varying outcomes
    // setting it to roll(2) results in essentially
    // a giant square maze
    if (ix == end_x && iy == end_y) {
      for (int y=starti; y<=starti+endi; y++) {
        for (int x=starti; x<=starti+endi; x++) {
          if (roll(200) == 1) {
            int count=0;
            for (int i=0; i<8; i++) {
              int tx = MAX(0, MIN(x + around[i][0], size));
              int ty = MAX(0, MIN(y + around[i][1], size));
              if (c->cells[ty][tx] != CELL_EMPTY)
                count++;
            }

            if (!count && c->cells[y][x] == CELL_EMPTY)
              index = dig_cells(l, index, x, y, roll(size / 2), tidyness, size);
          }
        }
      }

      break;
    }
  }

  // place cells
  for (int i=0; i<index; i++)
    if (c->cells[l[i].y][l[i].x] == CELL_EMPTY)
      c->cells[l[i].y][l[i].x] = CELL_HALL;

  // remove cells that are fully surrounded
  for (int i=0; i<=size; i++) {
    for (int y=0; y<=size; y++) {
      for (int x=0; x<=size; x++) {
        if (c->cells[y][x] != CELL_HALL)
          continue;

        int count=0;
        for (int i=0; i<8; i++) {
          int tx = MAX(0, MIN(x + around[i][0], size));
          int ty = MAX(0, MIN(y + around[i][1], size));
          if (c->cells[ty][tx] != CELL_EMPTY)
            count++;
        }

        // remove only if 7 or more around us are filled
        if (count >= 7 && roll(CHANCE_CELL_REMOVE) == 1)
          c->cells[y][x] = CELL_EMPTY;
      }
    }
  }

  // ---------------
  // now fill the map up with the
  // various cell types we need
  // ---------------

  // place treasure cells
  int treasure = 1 + MAX((size / 10) - 1, 0);
  int treasurei = 0;
  while (treasure > 0) {
    int x = -1, y = -1;

    int no[CELL_TYPES] = {0}, yes[CELL_TYPES] = {0};
    no[0]  = CELL_START;
    no[1]  = CELL_END;
    no[2]  = CELL_TREASURE;
    yes[0] = CELL_HALL;

    // pick a cell
    pick_cell(l, c, index, &x, &y, no, yes, 3, size);

    if (x >= 0 && y >= 0) {
      // place treasure cell
      c->cells[y][x] = CELL_TREASURE;

      // make it bigger..
      // roll(2) or 1 + roll(2) ?
      int rsize = 2;
      for (int ty=y-roll(rsize); ty<y+roll(rsize); ty++) {
        for (int tx=x-roll(rsize); tx<x+roll(rsize); tx++) {
          tx = MAX(0, MIN(tx, size));
          ty = MAX(0, MIN(ty, size));

          int t = c->cells[ty][tx];
          if (t == CELL_EMPTY || t == CELL_HALL)
            c->cells[ty][tx] = CELL_TREASURE;
        }
      }

      treasurei++;
      treasure--;
    } else {
      // uh oh, couldnt place any treasure cells
      free(l);
      return 0;
    }
  }

  // place power cell
  // int powerchance = roll(MAX(8 - (size / 2), 2)) == 1;
  int tx = -1, ty = -1;
  while (tx == -1 && ty == -1) {
    int x = -1, y = -1;

    int no[CELL_TYPES] = {0}, yes[CELL_TYPES] = {0};
    no[0]  = CELL_START;
    no[1]  = CELL_END;
    no[2]  = CELL_TREASURE;
    yes[0] = CELL_HALL;

    // pick a cell
    pick_cell(l, c, index, &x, &y, no, yes, 3, size);

    // place power cell
    if (x >= 0 && y >= 0) {
      c->cells[y][x] = CELL_POWER;
      break;
    }
  }

  // place rooms
  int roomindex = CELL_ROOM;
  for (int i=0; i<index; i++) {
    int x = l[i].x;
    int y = l[i].y;

    if (c->cells[y][x] != CELL_HALL)
      continue;

    // num surrounding rooms
    int j=0, count=0;
    for (; j<4; j++) {
      int tx = MAX(0, MIN(x + adjacent[j][0], size));
      int ty = MAX(0, MIN(y + adjacent[j][1], size));
      if (tx == x && ty == y)
        continue;

      int cell = c->cells[ty][tx];
      if (cell != CELL_EMPTY)
        count++;
    }

    if (!count)
      continue;

    if (count == 1 || (count <= 2 && roll(CHANCE_CELL_ROOM) == 1)) {
      c->cells[y][x] = roomindex;

      // make it bigger..
      // roll(2) or 1 + roll(2) ?
      int rsize = roll(2) + 1;
      for (int ty=y-(roll(rsize) - 1); ty<y+roll(rsize)-1; ty++) {
        for (int tx=x-(roll(rsize) - 1); tx<x+roll(rsize)-1; tx++) {
          tx = MAX(0, MIN(tx, size));
          ty = MAX(0, MIN(ty, size));

          int t = c->cells[ty][tx];
          if (t == CELL_EMPTY || t == CELL_HALL)
            c->cells[ty][tx] = roomindex;
        }
      }

      roomindex++;
    }
  }

  // place prefabs
  for (int i=0; i<index; i++) {
    int x = l[i].x;
    int y = l[i].y;

    if (c->cells[y][x] != CELL_HALL)
      continue;

    // num surrounding rooms
    int count=0;
    for (int j=0; j<8; j++) {
      int tx = MAX(0, MIN(x + around[j][0], size));
      int ty = MAX(0, MIN(y + around[j][1], size));
      if (tx == x && ty == y)
        continue;

      int cell = c->cells[ty][tx];
      if (cell == CELL_EMPTY)
        count++;
      if (cell == CELL_PREFAB) {
        count = 0;
        break;
      }
    }

    if (!count)
      continue;

    // lower chance to become prefab if a
    // lot of surrounding cells are present
    if (roll(count + roll(MAX(count / 2,CHANCE_CELL_PREFAB))) == 1) {
      c->cells[y][x] = CELL_PREFAB;

      int pcount = 0;
      for (int ty=y-1; ty<y+1; ty++) {
        for (int tx=x-1; tx<x+1; tx++) {
          int t = c->cells[MAX(0, MIN(ty, size))][MAX(0, MIN(tx, size))];

          // check if a prefab is around
          int prefab=0;
          for (int j=0; j<4; j++) {
            int ax = MAX(0, MIN(tx + adjacent[j][0], size));
            int ay = MAX(0, MIN(ty + adjacent[j][1], size));
            if (ax == x && ay == y)
              continue;

            int cell = c->cells[ay][ax];
            if (cell == CELL_PREFAB) {
              prefab = 1;
              break;
            }
          }

          if (prefab) {
            pcount = 0;
            break;
          }

          // no prefab, add to counter
          if (t == CELL_EMPTY || t == CELL_HALL)
            pcount++;
        }
      }

      if (pcount == 3) {
        for (int ty=y-1; ty<y+1; ty++) {
          for (int tx=x-1; tx<x+1; tx++) {
            int txb = MAX(0, MIN(tx, size));
            int tyb = MAX(0, MIN(ty, size));
            c->cells[tyb][txb] = CELL_PREFAB;
          }
        }
      }
    }
  }

  // place caves
  for (int i=0; i<index; i++) {
    int x = l[i].x;
    int y = l[i].y;

    if (c->cells[y][x] != CELL_HALL)
      continue;

    // num adjacent rooms
    int j=0, count=0;
    for (; j<4; j++) {
      int tx = MAX(0, MIN(x + adjacent[j][0], size));
      int ty = MAX(0, MIN(y + adjacent[j][1], size));
      if (tx == x && ty == y)
        continue;

      int cell = c->cells[ty][tx];
      if (cell != CELL_EMPTY)
        count++;
    }

    if (!count)
      continue;

    if (count >= 2 && count <= 4 && roll(CHANCE_CELL_CAVE) == 1) {
      c->cells[y][x] = CELL_CAVE;

      // walk around adjacent tiles
      // placing more caves
      for (int k=0; k<4; k++) {
        for (int j=0; j<4; j++) {
          int tx = MAX(0, MIN(x + adjacent[j][0], size));
          int ty = MAX(0, MIN(y + adjacent[j][1], size));
          if (tx == x && ty == y)
            continue;

          int cell = c->cells[ty][tx];
          if (cell == CELL_HALL && roll(CHANCE_CELL_CAVE) == 1) {
            c->cells[ty][tx] = CELL_CAVE;
            x = tx;
            y = ty;
          }
        }
      }
    }
  }

  // dissolve single hallway cells into surrounding cells
  for (int i=0; i<index; i++) {
    int x = l[i].x;
    int y = l[i].y;

    if (c->cells[y][x] != CELL_HALL)
      continue;

    // num adjacent rooms
    int count=0, merge=CELL_EMPTY;
    for (int j=0; j<4; j++) {
      int tx = MAX(0, MIN(x + adjacent[j][0], size));
      int ty = MAX(0, MIN(y + adjacent[j][1], size));
      if (tx == x && ty == y)
        continue;

      int cell = c->cells[ty][tx];
      if (cell == CELL_HALL)
        count++;
      else if (cell >= CELL_ROOM)
        merge = cell;
    }

    if (!count && merge >= CELL_ROOM)
      c->cells[y][x] = merge;
  }

  // clean up floating cells
  cells_t copy;
  memcpy(copy.cells, c->cells, sizeof(char) * CELL_MAX * CELL_MAX);

  typedef struct {
    int x, y, count;
  } sources_t;
  sources_t *sources = malloc(sizeof(sources_t) * CELL_MAX * CELL_MAX);
  int sourcei = 0;

  for (int y=0; y<=size; y++) {
    for (int x=0; x<=size; x++) {
      int count = 0;
      if (copy.cells[y][x] != CELL_FILL)
        count = fill_cells(x, y, CELL_FILL, size, &copy, 0);

      if (count) {
        sources[sourcei].x       = x;
        sources[sourcei].y       = y;
        sources[sourcei++].count = count;
      }
    }
  }

  // find largest area
  int sourcesize = 0, sourceindex = -1;
  for (int i=0; i<sourcei; i++) {
    if (sources[i].count > sourcesize) {
      sourcesize = sources[i].count;
      sourceindex = i;
    }
  }

  // fill all but largest area
  if (sourceindex >= 0) {
    for (int i=0; i<sourcei; i++) {
      if (i == sourceindex)
        continue;

      fill_cells(sources[i].x, sources[i].y, CELL_EMPTY, size, c, 0);
    }
  }
  free(sources);

  // remove floating cells from list
  ivec2_t *templist = malloc(sizeof(ivec2_t) * index);
  int tempindex = 0;
  for (int i=0; i<index; i++) {
    int lx = l[i].x;
    int ly = l[i].y;

    // push onto templist
    if (c->cells[ly][lx] != CELL_EMPTY) {
      templist[tempindex].x   = lx;
      templist[tempindex++].y = ly;
    }
  }
  memcpy(l, templist, sizeof(ivec2_t) * tempindex);
  index = tempindex;
  free(templist);

  c->num = index;
  free(l);
  return 1;
}
/* ---------------- */




// CHUNK STUFF
/* ---------------- */
#define CELL_SIZE 4

typedef struct {
  char *tiles;
  int width, height;
  int rwidth, rheight;
  int x, y;
} chunk_t;

int generate_fill(int *tiles, int x, int y, int w, int h, int a, int b, int count)
{
  if (x < 0 || y < 0 || x > w || y > h)
    return count;

  int index = (y * w) + x;
  if (index > w * h)
    return count;

  if (tiles[index] == a) {
    tiles[index] = b;
    count++;
  } else {
    return count;
  }

  count = generate_fill(tiles, x+1, y+0, w, h, a, b, count);
  count = generate_fill(tiles, x-1, y+0, w, h, a, b, count);
  count = generate_fill(tiles, x+0, y+1, w, h, a, b, count);
  count = generate_fill(tiles, x+0, y-1, w, h, a, b, count);

  return count;
}

int generate_room(chunk_t *c, int size, int open[4], int wall, int fill)
{
  int width  = size;
  int height = size;
  int adoor  = 1 + rand() % (size - 2);
  int bdoor  = 1 + rand() % (size - 2);
  int cdoor  = 1 + rand() % (size - 2);
  int ddoor  = 1 + rand() % (size - 2);
  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      c->tiles[(y*c->width)+x] = fill;

      // open sides
      if (x == 0 && open[2] != 1)
        c->tiles[(y*c->width)+x] = wall;
      if (x == width-1 && open[3] != 1)
        c->tiles[(y*c->width)+x] = wall;

      if (y == 0 && open[0] != 1)
        c->tiles[(y*c->width)+x] = wall;
      if (y == height-1 && open[1] != 1)
        c->tiles[(y*c->width)+x] = wall;

      // doors
      if (x == 0 && y == adoor && open[2] == 2)
        c->tiles[(y*c->width)+x] = 'D';
      if (x == width-1 && y == bdoor && open[3] == 2)
        c->tiles[(y*c->width)+x] = 'D';
      if (y == 0 && x == cdoor && open[0] == 2)
        c->tiles[(y*c->width)+x] = 'D';
      if (y == height-1 && x == ddoor && open[1] == 2)
        c->tiles[(y*c->width)+x] = 'D';
    }
  }

  c->rwidth  = width;
  c->rheight = height;

  return 1;
}

int generate_chunks(cells_t *c, chunk_t *level)
{
  // initialize a chunk per cell
  chunk_t *chunks = malloc(sizeof(chunk_t) * c->size * c->size);
  int chunkindex = 0;

  // have some temporary chunk that is used for
  // each iteration below, generate a room or
  // whatever and store it in this scratchpad
  // chunk, then copy the tile data from it
  // to one of the chunks in the array above
  // selecting only the smallest region of tiles
  // while throwing away empty space around it
  //
  // 1000x1000 tiles or something
  chunk_t temp;
  int temp_size = 100;
  temp.width    = temp_size;
  temp.height   = temp_size;
  temp.rwidth   = 0;
  temp.rheight  = 0;
  temp.tiles    = malloc(sizeof(char) * temp_size * temp_size);

  int size = c->size;

  // generate rooms, caves etc
  int width = 0, height = 0;
  for (int y=0; y<=size; y++) {
    for (int x=0; x<=size; x++) {
      int cell = c->cells[y][x];

      if (cell == CELL_EMPTY)
        continue;

      // clear temp chunk
      memset(temp.tiles, ' ', sizeof(char) * temp_size * temp_size);

      // find out where doors should be
      // up down left right
      int open[4] = {0};

      for (int i=0; i<4; i++) {
        int tx = MAX(0, MIN(x + adjacent[i][0], size));
        int ty = MAX(0, MIN(y + adjacent[i][1], size));
        int t = c->cells[ty][tx];

        if ((tx == x && ty == y) || t == CELL_EMPTY)
          continue;

        if (t == cell)
          open[i] = 1; // side is open
        else
          open[i] = 2; // side is door
      }

      if (cell == CELL_HALL)
        generate_room(&temp, CELL_SIZE, open, '#', '.');
      else
        generate_room(&temp, CELL_SIZE, open, '#', '!');

      // copy to chunk list
      chunks[chunkindex].width  = temp.rwidth;
      chunks[chunkindex].height = temp.rheight;
      chunks[chunkindex].x      = x * (CELL_SIZE - 1);
      chunks[chunkindex].y      = y * (CELL_SIZE - 1);
      chunks[chunkindex].tiles  = malloc(sizeof(char) * temp.rwidth * temp.rheight);
      for (int j=0; j<temp.rheight; j++) {
        for (int k=0; k<temp.rwidth; k++) {
          int ia = (j * temp.width) + k;
          int ib = (j * temp.rwidth) + k;
          chunks[chunkindex].tiles[ib] = temp.tiles[ia];
        }
      }

      width  = MAX(chunks[chunkindex].x + temp.rwidth, width);
      height = MAX(chunks[chunkindex].y + temp.rheight, height);

      chunkindex++;
    }
  }
  width++; height++;

  // iterate over chunks and copy them
  // to the map at the appropriate locations
  level->tiles  = malloc(sizeof(char) * width * height);
  memset(level->tiles, ' ', sizeof(char) * width * height);
  level->width  = width;
  level->height = height;
  for (int i=0; i<chunkindex; i++) {
    chunk_t *chunk = &chunks[i];
    for (int j=0; j<chunk->height; j++) {
      for (int k=0; k<chunk->width; k++) {
        int cx = chunk->x;
        int cy = chunk->y;

        int index = (j * chunk->width) + k;
        int lindex = (cy * level->width) + cx;

        lindex += (j * level->width) + k;

        int a = level->tiles[lindex];
        int b = chunk->tiles[index];

        // fix minor generation issue
        // where the bottom-right tile of a corner
        // is a floor tile not a wall tile
        if (!(a == '#' && b == '!') && !(a == '#' && b == '.'))
          level->tiles[lindex] = b;
      }
    }
  }

  // cut off empty space around level
  int minx = width, miny = height;
  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      if (level->tiles[(y*width)+x] != ' ') {
        minx = MIN(x, minx);
        miny = MIN(y, miny);
      }
    }
  }

  int rwidth  = (width-minx);
  int rheight = (height-miny);
  char *tmp = malloc(sizeof(char) * rwidth * rheight);
  memset(tmp, 0, sizeof(char) * rwidth * rheight);
  for (int y=0; y<(height-miny); y++)
    for (int x=0; x<(width-minx); x++)
      tmp[(y*rwidth)+x] = level->tiles[((miny+y)*width)+x+minx];
  free(level->tiles);
  level->tiles  = tmp;
  level->width  = rwidth;
  level->height = rheight;

  free(temp.tiles);
  for (int i=0; i<chunkindex; i++)
    free(chunks[i].tiles);
  free(chunks);

  // find rooms and set their ids
  int *roomids = malloc(sizeof(int) * level->width * level->height);
  memset(roomids, 0, sizeof(int) * level->width * level->height);
  for (int i=0; i<level->width*level->height; i++) {
    if (level->tiles[i] == '!' || level->tiles[i] == '.')
      roomids[i] = 1;
    if (level->tiles[i] == 'D')
      roomids[i] = -1;
  }

  int cur_id = 2;
  for (int y=0; y<level->height; y++) {
    for (int x=0; x<level->width; x++) {
      if (roomids[(y*level->width)+x] != 1)
        continue;

      generate_fill(roomids, x, y, level->width, level->height, 1, cur_id++, 0);
    }
  }
  int room_count = cur_id-2;

  typedef struct {
    int id, index, x, y;
  } node_t;
  typedef struct {
    int id, node_count;
    node_t nodes[50];
  } room_t;
  room_t *graph = malloc(sizeof(room_t) * room_count * room_count);
  memset(graph, 0, sizeof(room_t) * room_count * room_count);
  int graphi = 0;

  // create a graph of connected rooms
  // by iterating over the doors and checking the
  // rooms either side of it
  for (int y=0; y<level->height; y++) {
    for (int x=0; x<level->width; x++) {
      int index = (y*level->width)+x;

      // check if door (-1)
      if (roomids[index] != -1)
        continue;

      int a = 0, b = 0;
      int left  = (y*level->width)+x-1;
      int right = (y*level->width)+x+1;
      int up    = ((y-1)*level->width)+x;
      int down  = ((y+1)*level->width)+x;
      if (level->tiles[left] == '#' && level->tiles[right] == '#') {
        a = roomids[up];
        b = roomids[down];
      } else {
        a = roomids[left];
        b = roomids[right];
      }

      // add a and b to the graph if not already on it
      int ids[2] = {a, b};
      for (int i=0; i<2; i++) {
        // attempt to find the node
        int found_a = 0, found_b = 0, index = 0;
        for (int j=0; j<graphi; j++) {
          if (graph[j].id != ids[i])
            continue;

          found_a = 1;
          index = j;

          for (int k=0; k<graph[j].node_count; k++) {
            if (graph[j].nodes[k].id == ids[!i]) {
              found_b = 1;
              break;
            }
          }

          if (!found_b)
            break;
        }

        // newly found room
        if (!found_a) {
          // .index = graphi+1
          graph[graphi].id = ids[i];
          graph[graphi].nodes[graph[graphi].node_count].id = ids[!i];
          graph[graphi].nodes[graph[graphi].node_count].x  = x;
          graph[graphi].nodes[graph[graphi].node_count].y  = y;
          graph[graphi].node_count++;
          graphi++;
        }

        // old room, just update node list
        if (found_a && !found_b) {
          graph[index].nodes[graph[index].node_count].id = ids[!i];
          graph[index].nodes[graph[index].node_count].x  = x;
          graph[index].nodes[graph[index].node_count].y  = y;
          graph[index].node_count++;
        }
      }
    }
  }

  // convert ids to linear index
  for (int i=0; i<graphi; i++) {
    room_t *node = &graph[i];
    for (int j=0; j<node->node_count; j++) {
      int id = node->nodes[j].id;
      for (int k=0; k<graphi; k++) {
        if (graph[k].id == id) {
          node->nodes[j].index = k;
          break;
        }
      }
    }
  }

  // randomise the node lists
  for (int j=0; j<graphi; j++) {
    room_t *node = &graph[j];

    for (int i=0; i<node->node_count-1; i++) {
      size_t j = i + rand() / (RAND_MAX / (node->node_count  - i) + 1);
      node_t t = node->nodes[j];
      node->nodes[j] = node->nodes[i];
      node->nodes[i] = t;
    }
  }

  // generate the mst
  node_t *mst = malloc(sizeof(node_t) * room_count * room_count);
  memset(mst, 0, sizeof(node_t) * room_count * room_count);

  mst[0].id    = graph[0].id;
  mst[0].index = 0;
  int msti = 1;

  int done = 0;
  while (!done) {
    for (int n=0; n<msti; n++) {
      room_t *room = &graph[mst[n].index];

      int found = 0;
      node_t *conn = NULL;
      for (int i=0; i<room->node_count; i++) {
        conn = &room->nodes[i];

        // make sure its an unseen connection
        found = 0;
        for (int j=0; j<msti; j++) {
          if (mst[j].id == conn->id) {
            found = 1;
            break;
          }
        }

        if (!found)
          break;
      }

      if (!found && conn) {
        memcpy(&mst[msti], conn, sizeof(node_t));
        msti++;
        break;
      }
    }

    if (msti > graphi-1)
      done = 1;
  }

  // remove doorways not on the mst
  for (int y=0; y<level->height; y++) {
    for (int x=0; x<level->width; x++) {
      int index = (y*level->width)+x;

      if (level->tiles[index] != 'D')
        continue;

      int found = 0;
      for (int i=0; i<msti; i++) {
        if (mst[i].x == x && mst[i].y == y) {
          found = 1;
          break;
        }
      }

      // chance to leave looping pathways
      int loopchance = roll(10);
      if (!found && loopchance > 1)
        level->tiles[index] = '#';
    }
  }

  free(mst);
  free(graph);
  free(roomids);

  return 1;
}
/* ---------------- */


int gen(map_t *map)
{
  // generate initial level layout
  cells_t cells;
  while (!generate_cells(&cells, 40, 1)) {
    printf("Failed generating cells, retrying\n");
  }

  // generate chunks from layout
  chunk_t level;
  while (!generate_chunks(&cells, &level)) {
    printf("Failed generating chunks, retrying\n");
  }

  int set_start = 0, set_end = 0;
  for (;;) {
    int x = rand() % level.width;
    int y = rand() % level.height;
    if (level.tiles[(y*level.width)+x] != '!')
      continue;

    if (!set_start && roll(100) == 1) {
      map->sx = x;
      map->sy = y;
      set_start = 1;
    }
    if (!set_end && roll(100) == 1) {
      map->ex = x;
      map->ey = y;
      set_end = 1;
    }

    if (set_start && set_end)
      break;
  }

  if (map->tiles) {
    free(map->tiles);
    map->tiles = NULL;
  }

  // convert to appropriate tile values
  map->tiles = malloc(sizeof(uint8_t) * level.width * level.height);
  memset(map->tiles, 0, sizeof(uint8_t) * level.width * level.height);
  int max = level.width * level.height;

  for (int y=0; y<level.height; y++) {
    for (int x=0; x<level.width; x++) {
      int index = (y*level.width)+x;
      map->tiles[index] = 0;

      int left  = level.tiles[MAX(0, MIN((y*level.width)+x-1, max))];
      int right = level.tiles[MAX(0, MIN((y*level.width)+x+1, max))];
      int up    = level.tiles[MAX(0, MIN(((y-1)*level.width)+x, max))];
      int down  = level.tiles[MAX(0, MIN(((y+1)*level.width)+x, max))];

      switch (level.tiles[index]) {
        case '#': {
          int tile = TILE_STONE_VWALL;

          // top left corner
          if ((left == '#' || right == '#') && down != '#' && down != 'D')
            tile = TILE_STONE_HWALL;
          // if (up == '#' && down == 'D')
            // tile = TILE_STONE_HWALL;

          map->tiles[index] = tile;
          break;
        }
        case '!': {
          map->tiles[index] = TILE_WOOD_FLOOR;
          break;
        }
        case '.': {
          map->tiles[index] = TILE_STONE_FLOOR;
          break;
        }
        case 'D': {
          map->tiles[index] = TILE_DOOR_CLOSED;
          break;
        }
      }
    }
  }
  map->width  = level.width;
  map->height = level.height;

  free(level.tiles);

  return 1;
}