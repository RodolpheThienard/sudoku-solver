#define _GNU_SOURCE

#include "bitfield.h"
#include "wfc.h"

#include <omp.h>

bool
solve_cpu (wfc_blocks_ptr blocks)
{
  uint64_t iteration = 0;
  const uint64_t seed = blocks->seed;
  struct
  {
    uint32_t gy, x, y, _1;
    uint64_t state;
  } row_changes[blocks->grid_side];

  entropy_location min;
  entropy_location blk_entropy;
  vec2 grd_location;

  forever
  {
    bool changed = false;
    min.entropy = UINT8_MAX;
    // 1. Collapse
    for (uint32_t gx = 0; gx < blocks->grid_side; gx++){
        for (uint32_t gy = 0; gy < blocks->grid_side; gy++){
            blk_entropy = blk_min_entropy (blocks, gx, gy);
            if (blk_entropy.entropy < min.entropy){
                changed = true;
                min.entropy = blk_entropy.entropy;
                min.location.x = blk_entropy.location.x;
                min.location.y = blk_entropy.location.y;
                grd_location.x = gx;
                grd_location.y = gy;
            }
        }
    }

    uint64_t *blk_ptr = blk_at (blocks, grd_location.x, grd_location.y, min.location.x, min.location.y);
    *blk_ptr = entropy_collapse_state (*blk_ptr, grd_location.x, grd_location.y, min.location.x, min.location.y, seed, iteration);

    // 2. Propagate

    uint64_t collapsed = *blk_ptr;
    blk_propagate (blocks, grd_location.x, grd_location.y, min.location.x, min.location.y, collapsed);
    *blk_ptr = collapsed;

    // 3. Check Error
    iteration += 1;
    if (!changed)
      break;
  }

  return true;
}
