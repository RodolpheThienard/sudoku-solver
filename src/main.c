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
  const wfc_blocks_ptr init = wfc_load (0, args.data_file);

  int quit = 0;
  uint64_t iterations = 0;
  pthread_mutex_t seed_mtx = PTHREAD_MUTEX_INITIALIZER;

  int *volatile const quit_ptr = &quit;
  uint64_t *volatile const iterations_ptr = &iterations;

  const uint64_t max_iterations = count_seeds (args.seeds);
  const double start = omp_get_wtime ();
  bool already_solved = false;
  int output_solved = 0;

#pragma omp parallel
  while (!*quit_ptr)
    {
      wfc_blocks_ptr blocks = NULL;
      bool has_next_seed = true;
      uint64_t next_seed = 0;
#pragma omp critical
      {
        pthread_mutex_lock (&seed_mtx);
        next_seed = 0;
        has_next_seed = try_next_seed (&args.seeds, &next_seed);
        pthread_mutex_unlock (&seed_mtx);

        /* printf("seed: %lu - thread: %d - has_next_seed: %d\n", next_seed,
         * omp_get_thread_num(), has_next_seed); */
      }
      if (!has_next_seed)
        {
          break;
        }

      /* printf("Thread %d cloning into blocks\n", omp_get_thread_num()); */
      wfc_clone_into (&blocks, next_seed, init);
      /* printf("Thread %d attemping to solve using seed %lu\n",
       * omp_get_thread_num(), next_seed); */
      const bool solved = args.solver (blocks);
      /* printf("Thread %d ended solving using seed %lu\n",
       * omp_get_thread_num(), next_seed); */
      __atomic_add_fetch (iterations_ptr, 1, __ATOMIC_SEQ_CST);
      /* if (next_seed == 1 && !solved) */
      /*   { */
      /*       printf("Thread %d result on seed 1\n", omp_get_thread_num()); */
      /*       print_grd(blocks, 'v'); */
      /*   } */

#pragma omp critical
      {
        if (already_solved)
          {
            printf ("\nThread %d stopped and rejoy from the result of another "
                    "thread\n",
                    omp_get_thread_num ());
            __atomic_fetch_or (quit_ptr, true, __ATOMIC_SEQ_CST);
          }
        else if (solved && args.output_folder != NULL)
          {
            __atomic_fetch_or (quit_ptr, true, __ATOMIC_SEQ_CST);
            fputc ('\n', stdout);
            wfc_save_into (blocks, args.data_file, args.output_folder);
          }

        else if (solved)
          {
            printf ("\nSolved by thread %d using seed %lu\n",
                    omp_get_thread_num (), next_seed);
            __atomic_fetch_or (quit_ptr, true, __ATOMIC_SEQ_CST);
            // fputs ("\nsuccess with result:\n", stdout);
            print_grd (blocks, 'v');
            already_solved = true;
            output_solved = 1;
            /* abort (); */
          }

        else if (!*quit_ptr)
          {
            fprintf (stdout, "\r%.2f%% -> %.2fs - seed %lu",
                     ((double)(*iterations_ptr) / (double)(max_iterations))
                         * 100.0,
                     omp_get_wtime () - start, next_seed);
          }
      }

      if (solved)
        {
          break;
          output_solved = 1;
        }
    }

  return !output_solved;
}
