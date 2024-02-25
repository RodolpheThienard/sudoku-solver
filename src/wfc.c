#include <stdint.h>
#define _GNU_SOURCE

#include "bitfield.h"
#include "wfc.h"
/* #include "utils.h" */
#include "md5.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>


#pragma omp declare target
// Choose randomly the collapsed state of a block
uint64_t
entropy_collapse_state (uint64_t state, uint32_t gx, uint32_t gy, uint32_t x,
                        uint32_t y, uint64_t seed, uint64_t iteration)
{
  uint8_t *digest = (uint8_t *)malloc(16);
  for (int i = 0; i < 16; i++)
    {
      digest[i] = 0;
    }
  uint64_t random_number = 0;
  
    struct
    {
        uint32_t gx, gy, x, y;
        uint64_t seed, iteration;
    } random_state = {
        .gx = gx,
        .gy = gy,
        .x = x,
        .y = y,
        .seed = seed,
        .iteration = iteration,
    };
    uint32_t t = random_state.x ^ (random_state.x << 11);
    random_state.gy = random_state.gy ^ (random_state.gy >> 19) ^ t ^ (t >> 8);
    uint64_t z = (uint64_t)random_state.gx + random_state.iteration + random_state.x + random_state.y;
    z = (z ^ (z >> 21)) + (uint64_t)random_state.gy;
    z = (z ^ (z << 37)) + (uint64_t)random_state.seed + random_state.x - random_state.y;
    z = z ^ (z >> 4);
    z = z * 0x9e3779b97f4a7c13;
    z = z ^ (z >> 30);

  /* md5 ((uint8_t *)&random_state, sizeof (random_state), digest); */

  uint8_t entropy = entropy_compute (state);
  random_number = z % entropy;
  /* random_number = (*(uint64_t *)digest) % entropy; */

  for (uint8_t i = 0; i < 64; i++)
    {
      if (state & (1ULL << i))
        {
          if (random_number == 0)
            {
              free (digest);
              return (uint64_t)1 << i;
            }
          random_number--;
        }
    }
  free (digest);
  return 0;
}
#pragma omp end declare target

#pragma omp declare target
// Compute the entropy of a state
uint8_t
entropy_compute (uint64_t state)
{
  return __builtin_popcountll (state);
}
#pragma omp end declare target

#pragma omp declare target
void
wfc_clone_into (wfc_blocks_ptr *const restrict ret_ptr, uint64_t seed,
                const wfc_blocks_ptr blocks)
{
  const uint8_t grid_size = blocks->grid_side;
  const uint8_t block_size = blocks->block_side;
  wfc_blocks_ptr ret = *ret_ptr;

  const uint64_t size
      = ( block_size * block_size * grid_size * grid_size * sizeof (uint64_t));

  if (NULL == ret)
    {
      if (NULL == (ret = malloc (sizeof(wfc_blocks_ptr))))
        {
          /* fprintf (stderr, "failed to clone blocks structure\n"); */
          /* exit (EXIT_FAILURE); */
        }
      ret->states = NULL;
    }

  if (ret->states != NULL)
    {
      free (ret->states);
      ret->states = NULL;
    }
  ret->states = (uint64_t *)malloc (size);

  memcpy (ret->states, blocks->states, size);

  ret->grid_side = grid_size;
  ret->block_side = block_size;
  ret->seed = seed;
  *ret_ptr = ret;
}
#pragma omp end declare target

#pragma omp declare target
// Return the block with the minimum entropy
entropy_location
blk_min_entropy (const wfc_blocks_ptr blocks, uint32_t gx, uint32_t gy)
{
  entropy_location min;
  min.location.x = UINT32_MAX;
  min.location.y = UINT32_MAX;
  min.entropy = UINT8_MAX;

  for (uint32_t x = 0; x < blocks->block_side; x++)
    {
      for (uint32_t y = 0; y < blocks->block_side; y++)
        {
          uint64_t state = blocks->states[gx * blocks->grid_side * blocks->block_side * blocks->block_side + gy * blocks->block_side + x * blocks->block_side * blocks->grid_side + y];
          uint8_t entropy = entropy_compute (state);
          if (entropy > 1 && entropy <= min.entropy)
            {
              min.entropy = entropy;
              min.location.x = x;
              min.location.y = y;
            }
        }
    }
  return min;
}
#pragma omp end declare target

// Search for contradiction in the column
static inline bool
blk_filter_mask_for_column (wfc_blocks_ptr blocks, uint32_t gy, uint32_t y)
{
  uint64_t mask = 0;
  for (uint32_t gx = 0; gx < blocks->grid_side; gx++)
    {
      for (uint32_t x = 0; x < blocks->block_side; x++)
        {
          uint64_t *state = blk_at (blocks, gx, gy, x, y);
          if (mask & *state)
            {
              return true;
            }
          if (entropy_compute (*state) == 1)
            {
              mask |= *state;
            }
        }
    }
  return false;
}

// Search for contradiction in the row
static inline bool
blk_filter_mask_for_row (wfc_blocks_ptr blocks, uint32_t gy, uint32_t y)
{
  uint64_t mask = 0;
  for (uint32_t gx = 0; gx < blocks->grid_side; gx++)
    {
      for (uint32_t x = 0; x < blocks->block_side; x++)
        {
          uint64_t *state = blk_at (blocks, gx, gy, x, y);
          if (mask & *state)
            {
              return true;
            }
          if (entropy_compute (*state) == 1)
            {
              mask |= *state;
            }
        }
    }
  return false;
}

// Search for contradiction in the block
static inline bool
blk_filter_mask_for_block (wfc_blocks_ptr blocks, uint32_t gx, uint32_t gy)
{
  uint64_t mask = 0;
  for (uint32_t x = 0; x < blocks->block_side; x++)
    {
      for (uint32_t y = 0; y < blocks->block_side; y++)
        {
          uint64_t *state = blk_at (blocks, gx, gy, x, y);
          if (mask & *state)
            {
              return true;
            }
          if (entropy_compute (*state) == 1)
            {
              mask |= *state;
            }
        }
    }
  return false;
}

// -------------- TODO ---------------------
/* check if all state are different in the same column
   return false if no repetition (correct) else true */
bool
grd_check_error (wfc_blocks_ptr blocks)
{
  for (uint32_t gx = 0; gx < blocks->grid_side; gx++)
    {
      for (uint32_t gy = 0; gy < blocks->grid_side; gy++)
        {
          if (blk_filter_mask_for_block (blocks, gx, gy)
              || blk_filter_mask_for_row (blocks, gx, gy)
              || blk_filter_mask_for_column (blocks, gx, gy))
            {
              return true;
            }
        }
    }
  return false;
}

#pragma omp declare target
// Remove the collapsed value from the possible states in the column
static inline bool
grd_propagate_column (wfc_blocks_ptr blocks, uint32_t gx, uint32_t gy,
                      uint32_t x, uint32_t y, uint64_t collapsed,
                      vec2 *stack_blk, vec2 *stack_grd, uint32_t *idx)
{
  bool error = false;
  uint32_t old_entropy = 0;
  uint32_t new_entropy = 0;
/* #pragma omp distribute parallel for collapse(2) private(old_entropy, new_entropy) num_threads(blocks->grid_side * blocks->block_side) schedule(static) */
  for (uint32_t gx_tmp = 0; gx_tmp < blocks->grid_side; gx_tmp++)
    {
      for (uint32_t x_tmp = 0; x_tmp < blocks->block_side; x_tmp++)
        {
            if(error || gx_tmp == gx)
            {
              continue;
            }
          old_entropy
              = entropy_compute (blocks->states[gx_tmp * blocks->grid_side * blocks->block_side * blocks->block_side + gy * blocks->block_side + x_tmp * blocks->block_side * blocks->grid_side + y]);
          if (old_entropy > 1)
            {
              blocks->states[gx_tmp * blocks->grid_side * blocks->block_side * blocks->block_side + gy * blocks->block_side + x_tmp * blocks->block_side * blocks->grid_side + y] &= ~(collapsed);
              new_entropy
                  = entropy_compute (blocks->states[gx_tmp * blocks->grid_side * blocks->block_side * blocks->block_side + gy * blocks->block_side + x_tmp * blocks->block_side * blocks->grid_side + y]);
              if (new_entropy == 1)
                {
                  stack_blk[idx[0]].x = x_tmp;
                  stack_blk[idx[0]].y = y;
                  stack_grd[idx[0]].x = gx_tmp;
                  stack_grd[idx[0]].y = gy;
                  (idx[0])++;
                }
            }
          else if (collapsed == blocks->states[gx_tmp * blocks->grid_side * blocks->block_side * blocks->block_side + gy * blocks->block_side + x_tmp * blocks->block_side * blocks->grid_side + y] && (gx_tmp != gx || x_tmp != x))
            {
              error = true;
              continue;
            }
        }
    }
  blocks->states[gx * blocks->grid_side * blocks->block_side * blocks->block_side + gy * blocks->block_side + x * blocks->block_side * blocks->grid_side + y] = collapsed;
  return error;
}
#pragma omp end declare target

#pragma omp declare target
// Remove the collapsed value from the possible states in the row
static inline bool
grd_propagate_row (wfc_blocks_ptr blocks, uint32_t gx, uint32_t gy, uint32_t x,
                   uint32_t y, uint64_t collapsed, vec2 *stack_blk,
                   vec2 *stack_grd, uint32_t *idx)
{
  bool error = false;
  uint32_t old_entropy = 0;
  uint32_t new_entropy = 0;
/* #pragma omp distribute parallel for collapse(2) private(old_entropy, new_entropy) num_threads(blocks->grid_side * blocks->block_side) schedule(static) */
  for (uint32_t gy_tmp = 0; gy_tmp < blocks->grid_side; gy_tmp++)
    {
      for (uint32_t y_tmp = 0; y_tmp < blocks->block_side; y_tmp++)
        {
            if(error || gy_tmp == gy)
            {
              continue;
            }
          old_entropy
              = entropy_compute (blocks->states[gx * blocks->grid_side * blocks->block_side * blocks->block_side + gy_tmp * blocks->block_side + x * blocks->block_side * blocks->grid_side + y_tmp]);
          if (old_entropy > 1)
            {
              blocks->states[gx * blocks->grid_side * blocks->block_side * blocks->block_side + gy_tmp * blocks->block_side + x * blocks->block_side * blocks->grid_side + y_tmp] &= ~(collapsed);
              new_entropy
                  = entropy_compute (blocks->states[gx * blocks->grid_side * blocks->block_side * blocks->block_side + gy_tmp * blocks->block_side + x * blocks->block_side * blocks->grid_side + y_tmp]);
              if (new_entropy == 1)
                {
                  stack_blk[idx[0]].x = x;
                  stack_blk[idx[0]].y = y_tmp;
                  stack_grd[idx[0]].x = gx;
                  stack_grd[idx[0]].y = gy_tmp;
                  (idx[0])++;
                }
            }
          else if (collapsed == blocks->states[gx * blocks->grid_side * blocks->block_side * blocks->block_side + gy_tmp * blocks->block_side + x * blocks->block_side * blocks->grid_side + y_tmp] && (gy_tmp != gy || y_tmp != y))
            {
              error = true;
              continue;
            }
        }
    }
  blocks->states[gx * blocks->grid_side * blocks->block_side * blocks->block_side + gy * blocks->block_side + x * blocks->block_side * blocks->grid_side + y] = collapsed;
  return error;
}
#pragma omp end declare target

#pragma omp declare target
// Remove the collapsed value from the possible states in the block
static inline bool
blk_propagate (wfc_blocks_ptr blocks, uint32_t gx, uint32_t gy, uint32_t x,
               uint32_t y, uint64_t collapsed, vec2 *stack_blk,
               vec2 *stack_grd, uint32_t *idx)
{
  uint32_t old_entropy = 0;
  uint32_t new_entropy = 0;
  bool error = false;
  for (uint32_t x_tmp = 0; x_tmp < blocks->block_side; x_tmp++)
    {
      for (uint32_t y_tmp = 0; y_tmp < blocks->block_side; y_tmp++)
        {
            if(error || (x_tmp == x && y_tmp == y))
            {
              continue;
            }
          old_entropy
              = entropy_compute (blocks->states[gx * blocks->grid_side * blocks->block_side * blocks->block_side + gy * blocks->block_side + x_tmp * blocks->block_side * blocks->grid_side + y_tmp]);
          if (old_entropy > 1)
            {
              blocks->states[gx * blocks->grid_side * blocks->block_side * blocks->block_side + gy * blocks->block_side + x_tmp * blocks->block_side * blocks->grid_side + y_tmp] &= ~(collapsed);
              new_entropy
                  = entropy_compute (blocks->states[gx * blocks->grid_side * blocks->block_side * blocks->block_side + gy * blocks->block_side + x_tmp * blocks->block_side * blocks->grid_side + y_tmp]);
              if (new_entropy == 1)
                {
                  stack_blk[idx[0]].x = x_tmp;
                  stack_blk[idx[0]].y = y_tmp;
                  stack_grd[idx[0]].x = gx;
                  stack_grd[idx[0]].y = gy;
                  (idx[0])++;
                }
            }
          else if (collapsed == blocks->states[gx * blocks->grid_side * blocks->block_side * blocks->block_side + gy * blocks->block_side + x_tmp * blocks->block_side * blocks->grid_side + y_tmp] && (x_tmp != x || y_tmp != y))
            {
              error = true;
              continue;
            }
        }
    }
  blocks->states[gx * blocks->grid_side * blocks->block_side * blocks->block_side + gy * blocks->block_side + x * blocks->block_side * blocks->grid_side + y] = collapsed;
  return error;
}
#pragma omp end declare target

#pragma omp declare target
// remove the collapsed value from the possible states
// then stack and propagate the newly collapsed blocks
bool
all_propagate (wfc_blocks_ptr blocks, uint32_t gx, uint32_t gy, uint32_t x,
               uint32_t y, uint64_t collapsed)
{
  vec2 *stack_blk = (vec2 *)malloc (blocks->block_side * blocks->block_side
                                    * blocks->grid_side * sizeof (vec2));
  vec2 *stack_grd = (vec2 *)malloc (blocks->block_side * blocks->block_side
                                    * blocks->grid_side * sizeof (vec2));
  bool error = false; 
  uint32_t *idx = (uint32_t *)malloc (sizeof (uint32_t));
  idx[0] = 1;
  stack_blk[0].x = x;
  stack_blk[0].y = y;
  stack_grd[0].x = gx;
  stack_grd[0].y = gy;
  blocks->states[gx * blocks->grid_side * blocks->block_side * blocks->block_side + gy * blocks->block_side + x * blocks->block_side * blocks->grid_side + y] = collapsed;
  /* *blk_at (blocks, gx, gy, x, y) = collapsed; */
  while (idx[0] && !error)
    {
      idx[0]--;
      uint32_t cur_x = stack_blk[idx[0]].x;
      uint32_t cur_y = stack_blk[idx[0]].y;
      uint32_t cur_gx = stack_grd[idx[0]].x;
      uint32_t cur_gy = stack_grd[idx[0]].y;

      /* uint64_t cur_collapsed = *blk_at (blocks, cur_gx, cur_gy, cur_x, cur_y); */
      uint64_t cur_collapsed = blocks->states[cur_gx * blocks->grid_side * blocks->block_side * blocks->block_side + cur_gy * blocks->block_side + cur_x * blocks->block_side * blocks->grid_side + cur_y];

      error |= blk_propagate (blocks, cur_gx, cur_gy, cur_x, cur_y, cur_collapsed,
                           stack_blk, stack_grd, idx);
      error |= grd_propagate_column (blocks, cur_gx, cur_gy, cur_x, cur_y,
                                  cur_collapsed, stack_blk, stack_grd, idx);
      error |= grd_propagate_row (blocks, cur_gx, cur_gy, cur_x, cur_y,
                               cur_collapsed, stack_blk, stack_grd, idx);

      /* *blk_at (blocks, cur_gx, cur_gy, cur_x, cur_y) = cur_collapsed; */
      blocks->states[cur_gx * blocks->grid_side * blocks->block_side * blocks->block_side + cur_gy * blocks->block_side + cur_x * blocks->block_side * blocks->grid_side + cur_y] = cur_collapsed;
    }
  free (idx);
  free (stack_blk);
  free (stack_grd);
  return error;
}
#pragma omp end declare target
