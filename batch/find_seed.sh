#!/bin/bash
#SBATCH --job-name=sudoku-solver   # Nom de la tâche
#SBATCH --error=err/mon_programme_%A_%a.err    # Fichier d'erreur (%A: JobID, %a: ArrayID)
#SBATCH --output=err/mon_programme_%A_%a.out   # Fichier de sortie (%A: JobID, %a: ArrayID)
#SBATCH --array=1-200   # Nombre total d'itérations
#SBATCH -p short
#SBATCH -c 24

module load gcc/10.2.0

begin=$(($SLURM_ARRAY_TASK_ID*10000000000  ))
end=$((($SLURM_ARRAY_TASK_ID+1)*10000000000 ))
OMP_NUM_THREADS=24 ./build/wfc -s$begin-$end data/empty-6x6.data >> first/first$SLURM_ARRAY_TASK_ID.dat

