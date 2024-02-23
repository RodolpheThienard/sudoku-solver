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

// Choose randomly the collapsed state of a block
uint64_t
entropy_collapse_state (uint64_t state, uint32_t gx, uint32_t gy, uint32_t x,
                        uint32_t y, uint64_t seed, uint64_t iteration)
{
  uint8_t digest[16] = { 0 };
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

  md5 ((uint8_t *)&random_state, sizeof (random_state), digest);

  uint8_t entropy = entropy_compute (state);
  random_number = (*(uint64_t *)digest) % entropy;

  for (uint8_t i = 0; i < 64; i++)
    {
      if (state & (1ULL << i))
        {
          if (random_number == 0)
            {
              return (uint64_t)1 << i;
            }
          random_number--;
        }
    }
  return 0;
}

// Compute the entropy of a state
uint8_t
entropy_compute (uint64_t state)
{
  return __builtin_popcount (state);
}

void
wfc_clone_into (wfc_blocks_ptr *const restrict ret_ptr, uint64_t seed,
                const wfc_blocks_ptr blocks)
{
  const uint64_t grid_size = blocks->grid_side;
  const uint64_t block_size = blocks->block_side;
  wfc_blocks_ptr ret = *ret_ptr;

  const uint64_t size
      = (wfc_control_states_count (grid_size, block_size) * sizeof (uint64_t))
        + (grid_size * grid_size * block_size * block_size * sizeof (uint64_t))
        + sizeof (wfc_blocks);

  if (NULL == ret)
    {
      if (NULL == (ret = malloc (size)))
        {
          fprintf (stderr, "failed to clone blocks structure\n");
          exit (EXIT_FAILURE);
        }
    }
  else if (grid_size != ret->grid_side || block_size != ret->block_side)
    {
      fprintf (stderr, "size mismatch!\n");
      exit (EXIT_FAILURE);
    }

  memcpy (ret, blocks, size);
  ret->seed = seed;
  *ret_ptr = ret;
}

// Return the block with the minimum entropy
entropy_location
blk_min_entropy (const wfc_blocks_ptr blocks, uint32_t gx, uint32_t gy)
{
  entropy_location min;
  min.entropy = UINT8_MAX;

  for (uint32_t x = 0; x < blocks->block_side; x++)
    {
      for (uint32_t y = 0; y < blocks->block_side; y++)
        {
          uint64_t state = *blk_at (blocks, gx, gy, x, y);
          uint8_t entropy = entropy_compute (state); if (entropy > 1 && entropy <= min.entropy)
            {
              min.entropy = entropy;
              min.location.x = x;
              min.location.y = y;
            }
        }
    }
  return min;
}

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
  /* uint64_t len = blocks->grid_side * blocks->block_side; */
  /* bool *check_tab = calloc (len, sizeof (bool)); */
  /* uint64_t *start_column = grd_at (blocks, 0, gy); */
  /* for (uint32_t j = 0; j < len; j++) */
  /*   { */
  /*     uint64_t cell = check_tab[*start_column + (j * len)]; */
  /*     if (!cell) */
  /*       return true; */
  /*     check_tab[*start_column + (j * len)] = true; */
  /*   } */

  /* free (check_tab); */
  /* return false; */
}

// Remove the collapsed value from the possible states in the column
static inline bool
grd_propagate_column (wfc_blocks_ptr blocks, uint32_t gx, uint32_t gy,
                      uint32_t x, uint32_t y, uint64_t collapsed,
                      vec2 *stack_blk, vec2 *stack_grd, uint32_t *idx)
{
  bool error = false;
  uint32_t old_entropy = 0;
  uint32_t new_entropy = 0;
  for (uint32_t gx_tmp = 0; gx_tmp < blocks->grid_side; gx_tmp++)
    {
      for (uint32_t x_tmp = 0; x_tmp < blocks->block_side; x_tmp++)
        {
          old_entropy
              = entropy_compute (*blk_at (blocks, gx_tmp, gy, x_tmp, y));
          if (old_entropy > 1)
            {
              *blk_at (blocks, gx_tmp, gy, x_tmp, y) &= ~(collapsed);
              new_entropy
                  = entropy_compute (*blk_at (blocks, gx_tmp, gy, x_tmp, y));
              if (new_entropy == 1)
                {
                  stack_blk[*idx].x = x_tmp;
                  stack_blk[*idx].y = y;
                  stack_grd[*idx].x = gx_tmp;
                  stack_grd[*idx].y = gy;
                  (*idx)++;
                }
            }
          else if (collapsed == *blk_at (blocks, gx_tmp, gy, x_tmp, y)
                   && (gx_tmp != gx || x_tmp != x))
            {
              error = true;
              break;
            }
        }
    }
  *blk_at (blocks, gx, gy, x, y) = collapsed;
  return error;
}

// Remove the collapsed value from the possible states in the row
static inline bool
grd_propagate_row (wfc_blocks_ptr blocks, uint32_t gx, uint32_t gy, uint32_t x,
                   uint32_t y, uint64_t collapsed, vec2 *stack_blk,
                   vec2 *stack_grd, uint32_t *idx)
{
  bool error = false;
  uint32_t old_entropy = 0;
  uint32_t new_entropy = 0;
  for (uint32_t gy_tmp = 0; gy_tmp < blocks->grid_side; gy_tmp++)
    {
      for (uint32_t y_tmp = 0; y_tmp < blocks->block_side; y_tmp++)
        {
          old_entropy
              = entropy_compute (*blk_at (blocks, gx, gy_tmp, x, y_tmp));
          if (old_entropy > 1)
            {
              *blk_at (blocks, gx, gy_tmp, x, y_tmp) &= ~(collapsed);
              new_entropy
                  = entropy_compute (*blk_at (blocks, gx, gy_tmp, x, y_tmp));
              if (new_entropy == 1)
                {
                  stack_blk[*idx].x = x;
                  stack_blk[*idx].y = y_tmp;
                  stack_grd[*idx].x = gx;
                  stack_grd[*idx].y = gy_tmp;
                  (*idx)++;
                }
            }
          else if (collapsed == *blk_at (blocks, gx, gy_tmp, x, y_tmp)
                   && (gy_tmp != gy || y_tmp != y))
            {
              error = true;
              break;
            }
        }
    }
  *blk_at (blocks, gx, gy, x, y) = collapsed;
  return error;
}

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
          old_entropy
              = entropy_compute (*blk_at (blocks, gx, gy, x_tmp, y_tmp));
          if (old_entropy > 1)
            {
              *blk_at (blocks, gx, gy, x_tmp, y_tmp) &= ~(collapsed);
              new_entropy
                  = entropy_compute (*blk_at (blocks, gx, gy, x_tmp, y_tmp));
              if (new_entropy == 1)
                {
                  stack_blk[*idx].x = x_tmp;
                  stack_blk[*idx].y = y_tmp;
                  stack_grd[*idx].x = gx;
                  stack_grd[*idx].y = gy;
                  (*idx)++;
                }
            }
          else if (collapsed == *blk_at (blocks, gx, gy, x_tmp, y_tmp) &&
                   (x_tmp != x || y_tmp != y))
            {
              error = true;
              break;
            }
        }
    }
  *blk_at (blocks, gx, gy, x, y) = collapsed;
  return error;
}

// remove the collapsed value from the possible states
// then stack and propagate the newly collapsed blocks
bool
all_propagate (wfc_blocks_ptr blocks, uint32_t gx, uint32_t gy, uint32_t x,
               uint32_t y, uint64_t collapsed)
{
  vec2 stack_blk[blocks->block_side * blocks->block_side * blocks->grid_side];
  vec2 stack_grd[blocks->block_side * blocks->block_side * blocks->grid_side];
  bool error = false; 
  uint32_t idx = 1;
  stack_blk[0].x = x;
  stack_blk[0].y = y;
  stack_grd[0].x = gx;
  stack_grd[0].y = gy;
  *blk_at (blocks, gx, gy, x, y) = collapsed;
  while (idx && !error)
    {
      idx--;
      uint32_t cur_x = stack_blk[idx].x;
      uint32_t cur_y = stack_blk[idx].y;
      uint32_t cur_gx = stack_grd[idx].x;
      uint32_t cur_gy = stack_grd[idx].y;

      uint64_t cur_collapsed = *blk_at (blocks, cur_gx, cur_gy, cur_x, cur_y);

      error |= blk_propagate (blocks, cur_gx, cur_gy, cur_x, cur_y, cur_collapsed,
                           stack_blk, stack_grd, &idx);
      error |= grd_propagate_column (blocks, cur_gx, cur_gy, cur_x, cur_y,
                                  cur_collapsed, stack_blk, stack_grd, &idx);
      error |= grd_propagate_row (blocks, cur_gx, cur_gy, cur_x, cur_y,
                               cur_collapsed, stack_blk, stack_grd, &idx);

      *blk_at (blocks, cur_gx, cur_gy, cur_x, cur_y) = cur_collapsed;
    }
  return error;
}
