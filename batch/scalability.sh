#!/bin/bash
#SBATCH --job-name=sudoku-solver   # Nom de la tâche
#SBATCH --error=err/mon_programme_%A_%a.err    # Fichier d'erreur (%A: JobID, %a: ArrayID)
#SBATCH --output=err/mon_programme_%A_%a.out   # Fichier de sortie (%A: JobID, %a: ArrayID)
#SBATCH --array=1-24   # Nombre total d'itérations
#SBATCH -p short
#SBATCH -c 24

module load gcc/10.2.0

OMP_NUM_THREADS=$SLURM_ARRAY_TASK_ID ./build/wfc -s0-1000000 data/empty-6x6.data >> scalability/scalability$SLURM_ARRAY_TASK_ID.dat

echo "$SLURM_ARRAY_TASK_ID;$(cat scalability/scalability$SLURM_ARRAY_TASK_ID.dat)" >> scalability.dat
