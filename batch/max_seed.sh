#!/bin/bash
#SBATCH --job-name=sudoku-solver   # Nom de la tâche
#SBATCH --error=err/mon_programme_%A_%a.err    # Fichier d'erreur (%A: JobID, %a: ArrayID)
#SBATCH --output=err/mon_programme_%A_%a.out   # Fichier de sortie (%A: JobID, %a: ArrayID)
#SBATCH --array=3-5   # Nombre total d'itérations
#SBATCH -p short
#SBATCH -c 24

OMP_NUM_THREADS=1 ./build/wfc -s0-10000000000 data/empty-$SLURM_ARRAY_TASK_ID.data >> max_seed$SLURM_ARRAY_TASK_ID.dat

echo "$SLURM_ARRAY_TASK_ID;$(cat max_seed$SLURM_ARRAY_TASK_ID.dat)" >> max_seed.dat

#OMP_NUM_THREADS=1 ./build/wfc -s0-10000000000 data/nondet-simple-5x5.data >> max_seedSimple.da
