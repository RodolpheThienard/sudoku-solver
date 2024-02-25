#include "types.h"
#define _GNU_SOURCE

#include "bitfield.h"
#include "wfc.h"

#include <omp.h>


/* TODO:
 * 1. Make md5 work
 * 2. Make the propagate work
 * 3. send multiple seeds to be treated on different teams */

bool
solve_target (wfc_blocks_ptr blocks)
{
  uint64_t iteration = 0;
  const uint64_t seed = blocks->seed;
  /* printf("Seed: %lu\n", seed); */

  entropy_location min;
  min.entropy = UINT8_MAX;
  min.location.x = 0;
  min.location.y = 0;
  bool error = false;
  bool device = false;
  uint32_t grid_side = blocks->grid_side;
  uint32_t block_side = blocks->block_side;
  bool changed = true;

    // 1. Collapse
#pragma omp target map(to: iteration, seed, blocks->grid_side, blocks->block_side, block_side, grid_side) map(tofrom: error, device, changed, blocks->states[0:grid_side*grid_side*block_side*block_side], min) 
  {
#pragma omp teams num_teams(1)
      {
          /* if (omp_is_initial_device()) */
          /* { */
          /*     device = false; */
          /* } */
          /* else */
          /* { */
          /*     device = true; */
          /* } */
          entropy_location blk_entropy;
          blk_entropy.entropy = 0;
          blk_entropy.location.x = 0;
          blk_entropy.location.y = 0;
          vec2 grd_location;
          grd_location.x = 0;
          grd_location.y = 0;
          entropy_location* mins = (entropy_location *)malloc(grid_side * grid_side * sizeof(entropy_location));
          for (uint32_t i = 0; i < blocks->grid_side * blocks->grid_side; i++)
          {
              mins[i].entropy = UINT8_MAX;
              mins[i].location.x = 0;
              mins[i].location.y = 0;
          }
          /* printf("Changed: %d - Error: %d\n", changed, error); */
          while (changed && !error)
          {
              min.entropy = UINT8_MAX;
#pragma omp distribute parallel for collapse(2) private(blk_entropy)
              for (uint32_t gx = 0; gx < grid_side; gx++)
              {
                  for (uint32_t gy = 0; gy < grid_side; gy++)
                  {
                      mins[gx * grid_side + gy] = blk_min_entropy (blocks, gx, gy);
                  }
              }
              changed = false;
              for (uint32_t gx = 0; gx < grid_side; gx++)
              {
                  for (uint32_t gy = 0; gy < grid_side; gy++)
                  {
                      if (mins[gx * grid_side + gy].entropy <= min.entropy && ~mins[gx * grid_side + gy].location.x)
                      {
                          changed |= true;
                          min.entropy = mins[gx * grid_side + gy].entropy;
                          min.location.x = mins[gx * grid_side + gy].location.x;
                          min.location.y = mins[gx * grid_side + gy].location.y;
                          grd_location.x = gx;
                          grd_location.y = gy;
                      }
                  }
              }
              /* printf("Thread %d - Iteration %lu - seed %lu changed %d error %d\n", omp_get_thread_num(), iteration, seed, changed, error); */
              /* printf("Coords Grid %d %d - Block %d %d\n", grd_location.x, grd_location.y, min.location.x, min.location.y); */
              /* printf("Entropy %d\n", min.entropy); */
              /* printf("state %d\n", *blk_ptr); */
              uint64_t collapsed = entropy_collapse_state (blocks->states[grd_location.x * grid_side * block_side *block_side
                                                            + grd_location.y * block_side
                                                            + min.location.x * grid_side * block_side
                                                            + min.location.y] , grd_location.x,
                                                            grd_location.y, min.location.x,
                                                            min.location.y, seed, iteration);

              // 2. Propagate
              error = all_propagate (blocks, grd_location.x, grd_location.y, min.location.x, min.location.y, collapsed);
              /* printf("Iteration %lu - seed %lu changed %d\n", iteration, seed, changed); */
              iteration += 1;
          }
          free(mins);
      }
  }

    /* printf("Iteration %d - seed %d changed %d\n", iteration, seed, changed); */
    /* if (device) { printf("Running on device\n"); } */
    /* else        { printf("Running on host\n"); } */
    /* print_grd(blocks, 'v'); */



    /* printf("Seed %lu - Iteration %lu - Error %lu - Changed %d\n", seed, iteration, error, min.entropy); */
    /* print_grd(blocks, 'b'); */

    // 3. Check Error

    /* if (device) { printf("Ran on device\n"); } */
    /* else        { printf("Ran on host\n"); } */

  return !error;
}


