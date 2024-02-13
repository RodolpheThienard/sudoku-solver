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
  ret->states[0] = seed;
  *ret_ptr = ret;
}

// Return the block with the minimum entropy
uint64_t
blk_min_entropy (const wfc_blocks_ptr blocks, uint32_t gx, uint32_t gy)
{
  vec2 the_location = { 0 };
  uint8_t min_entropy = UINT8_MAX;

  uint64_t *grid_pos = grd_at (blocks, gx, gy);

  for (uint32_t x = 0; x < blocks->block_side; x++)
    {
      for (uint32_t y = 0; y < blocks->block_side; y++)
        {
          uint64_t state
              = *grid_pos + (x * blocks->block_side * blocks->grid_side + y);
          uint8_t entropy = entropy_compute (state);
          if (entropy < min_entropy)
            {
              min_entropy = entropy;
              the_location.x = x;
              the_location.y = y;
            }
        }
    }
  return *grid_pos + the_location.x * blocks->block_side * blocks->grid_side
         + the_location.y;
}

static inline uint64_t
blk_filter_mask_for_column (wfc_blocks_ptr blocks, uint32_t gy, uint32_t y,
                            uint64_t collapsed)
{
  return 0;
}

static inline uint64_t
blk_filter_mask_for_row (wfc_blocks_ptr blocks, uint32_t gx, uint32_t x,
                         uint64_t collapsed)
{
  return 0;
}

static inline uint64_t
blk_filter_mask_for_block (wfc_blocks_ptr blocks, uint32_t gy, uint32_t gx,
                           uint64_t collapsed)
{
  return 0;
}

/* check if all state are different in the same column
   return false if no repetition (correct) else true */
bool
grd_check_error_in_column (wfc_blocks_ptr blocks, uint32_t gy)
{
  uint64_t len = blocks->grid_side * blocks->block_side;
  bool *check_tab = calloc (len, sizeof (bool));
  uint64_t *start_column = grd_at (blocks, 0, gy);
  for (uint32_t j = 0; j < len; j++)
    {
      uint64_t cell = check_tab[*start_column + (j * len)];
      if (!cell)
        return true;
      check_tab[*start_column + (j * len)] = true;
    }

  free (check_tab);
  return false;
}

void
blk_propagate (wfc_blocks_ptr blocks, uint32_t gx, uint32_t gy,
               uint64_t collapsed)
{
  return 0;
}

void
grd_propagate_row (wfc_blocks_ptr blocks, uint32_t gx, uint32_t gy, uint32_t x,
                   uint32_t y, uint64_t collapsed)
{
  return 0;
}

void
grd_propagate_column (wfc_blocks_ptr blocks, uint32_t gx, uint32_t gy,
                      uint32_t x, uint32_t y, uint64_t collapsed)
{
  return 0;
}
