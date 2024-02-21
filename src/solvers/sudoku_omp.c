#define _GNU_SOURCE

#include "bitfield.h"
#include "wfc.h"

#include <omp.h>

bool
solve_openmp (wfc_blocks_ptr blocks)
{
  uint64_t iteration = 0;
  const uint64_t seed = blocks->seed;

  entropy_location blk_entropy = { 0, 0 };
  bool error = false;

  printf("Solver using OpenMP\n");
  forever
  {
    bool changed = false;
#pragma omp parallel for collapse(2) shared(blocks, changed) private(blk_entropy) num_threads(blocks->grid_side*blocks->grid_side)
    for (uint32_t gx = 0; gx < blocks->grid_side; gx++)
      {
        for (uint32_t gy = 0; gy < blocks->grid_side; gy++)
          {
            blk_entropy = blk_min_entropy (blocks, gx, gy);
            if (blk_entropy.entropy > 1 && blk_entropy.entropy < UINT8_MAX)
              {
                changed = true;
                // 1. Collapse
                uint64_t *blk_ptr = blk_at (blocks, gx, gy,
                        blk_entropy.location.x, blk_entropy.location.y);
                *blk_ptr = entropy_collapse_state (*blk_ptr, gx,
                                                   gy, blk_entropy.location.x,
                                                   blk_entropy.location.y, seed, iteration);
                // 2. Propagate
                uint64_t collapsed = *blk_ptr;
                error = all_propagate (blocks, gx, gy, blk_entropy.location.x,
                               blk_entropy.location.y, collapsed);
              }
          }
      }

    iteration += 1;
    if (!changed || error)
      {
        break;
      }
  }

  return !error;
}

