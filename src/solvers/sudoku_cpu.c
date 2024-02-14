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

  vec2 blk_location = { 0 };
  vec2 grd_location = { 0 };

  uint8_t min_entropy = UINT8_MAX;
  uint8_t entropy = 0;


  forever
  {
    bool changed = false;
    min_entropy = UINT8_MAX;
    // 1. Collapse
    for (uint32_t gx = 0; gx < blocks->grid_side; gx++){
        for (uint32_t gy = 0; gy < blocks->grid_side; gy++){
            for (uint32_t x = 0; x < blocks->block_side; x++){
                for (uint32_t y = 0; y < blocks->block_side; y++){
                    entropy = entropy_compute (*blk_at (blocks, gx, gy, x, y));
                    if (entropy > 1 && entropy < min_entropy){
                        changed = true;
                        min_entropy = entropy;
                        blk_location.x = x;
                        blk_location.y = y;
                        grd_location.x = gx;
                        grd_location.y = gy;
                    }
                }
            }
        }
    }

    uint64_t *blk_ptr = blk_at (blocks, grd_location.x, grd_location.y, blk_location.x, blk_location.y);
    *blk_ptr = entropy_collapse_state (*blk_ptr, grd_location.x, grd_location.y, blk_location.x, blk_location.y, seed, iteration);

    // 2. Propagate

    uint64_t collapsed = *blk_ptr;
    blk_propagate (blocks, grd_location.x, grd_location.y, blk_location.x, blk_location.y, collapsed);
    *blk_ptr = collapsed;

    // 3. Check Error


    iteration += 1;
    if (!changed)
      break;
  }

  return true;
}
