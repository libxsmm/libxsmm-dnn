#!/usr/bin/env bash

#SBATCH -J allconv_f32_ppad_skx_50_28thr_fwd
#SBATCH --get-user-env
#SBATCH --partition=skx
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=112
#SBATCH --time=24:00:00

export OMP_NUM_THREADS=28
export KMP_AFFINITY=granularity=fine,compact,1,28
export ITERS=1
#export LIBXSMM_VERBOSE=-1

date
srun ./run_convs.sh 50 ${ITERS} -1 f32 F 1 0 64 64 0
date
