#define _GNU_SOURCE

#include "bitfield.h"
#include "wfc.h"

#include <float.h>
#include <math.h>
#include <omp.h>
#include <pthread.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
  omp_set_dynamic (false);

  wfc_args args = wfc_parse_args (argc, argv);
  wfc_blocks_ptr init = wfc_load (0, args.data_file);

  int quit = 0;
  uint64_t iterations = 0;

  int *volatile const quit_ptr = &quit;
  uint64_t *volatile const iterations_ptr = &iterations;

  const uint64_t max_iterations = count_seeds (args.seeds);
  const double start = omp_get_wtime ();
  int output_solved = 0;

  if (args.solver == solve_cpu) { omp_set_num_threads (1); }
  else                          { omp_set_num_threads ((int32_t)args.parallel); }

  uint64_t begin = 0;
  uint64_t end = 0;
  get_seed_boundaries(args.seeds, &begin, &end);

  /* for (uint64_t cur_seed = begin; cur_seed < end; cur_seed++) */
#pragma omp parallel 
  while(!*quit_ptr)
  {
      uint64_t next_seed = 0;
      bool has_next_seed = true;
#pragma omp critical
      {
          next_seed = 0;
          has_next_seed = try_next_seed(&args.seeds, &next_seed);
          
      }
      if(!has_next_seed)
      {
          __atomic_fetch_or (quit_ptr, true, __ATOMIC_SEQ_CST);
          continue;
      }

      wfc_blocks_ptr blocks = NULL;
      

      wfc_clone_into (&blocks, next_seed, init);
      const bool solved = args.solver (blocks);
      __atomic_add_fetch (iterations_ptr, 1, __ATOMIC_SEQ_CST);

#pragma omp critical
      {
          if (*quit_ptr)
          {
              /* printf ("\nThread %d stopped and rejoy from the result of another " */
              /*         "thread\n", */
              /*         omp_get_thread_num ()); */
          }
          else if (solved && args.output_folder != NULL)
          {
              __atomic_fetch_or (quit_ptr, true, __ATOMIC_SEQ_CST);
              fputc ('\n', stdout);
              printf ("\nSolved by thread %d using seed %lu\n",
                      omp_get_thread_num (), next_seed);
              print_grd (blocks, 'v');
              output_solved = 1;
              wfc_save_into (blocks, args.data_file, args.output_folder);
          }
          else if (solved)
          {
              printf ("\nSolved by thread %d using seed %lu\n",
                      omp_get_thread_num (), next_seed);
              __atomic_fetch_or (quit_ptr, true, __ATOMIC_SEQ_CST);
              fputs ("\nsuccess with result:\n", stdout);
              print_grd (blocks, 'v');
              if(grd_check_error (blocks))
                {
                  printf("Error in the result\n");
                }
              else
                {
                  printf("No error in the result or unfinished\n");
                }
              output_solved = 1;
              /* abort (); */
          }
          else if (!*quit_ptr)
          {
              fprintf (stdout, "\r%.2f%% -> %.2fs - seed %lu - %d threads",
                       ((double)(*iterations_ptr) / (double)(max_iterations))
                           * 100.0,
                       omp_get_wtime () - start, next_seed, omp_get_num_threads ());
          }
      }
  }  

  free (init->states);
  free(init);

  return !output_solved;
}
