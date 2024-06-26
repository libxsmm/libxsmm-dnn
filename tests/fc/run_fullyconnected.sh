#!/usr/bin/env bash
set -eo pipefail

HERE=$(cd "$(dirname "$0")" && pwd -P)
UNAME=$(command -v uname)
SORT=$(command -v sort)
GREP=$(command -v grep)
CUT=$(command -v cut)
WC=$(command -v wc)
TR=$(command -v tr)
NUMA=-1

if [ ! "${CHECK}" ] || [ "0" = "${CHECK}" ]; then
  if [ ! "${CHECK_DNN_MB}" ]; then CHECK_DNN_MB=64; fi
  if [ ! "${CHECK_DNN_ITERS}" ]; then CHECK_DNN_ITERS=1000; fi
else # check
  if [ ! "${CHECK_DNN_MB}" ]; then CHECK_DNN_MB=64; fi
  if [ ! "${CHECK_DNN_ITERS}" ]; then CHECK_DNN_ITERS=1; fi
fi

if [ $# -ne 8 ]
then
  echo "Usage: $(basename $0) bin=(f32, bf16, bf8) iters MB type=(A, F, B, U, M) fuse=(0 (None), 1 (Bias), 2 (ReLU), 4 (Bias+ReLU)) bn bc bk vnnipack"
  BIN=f32
  ITERS=${CHECK_DNN_ITERS}
  MB=${CHECK_DNN_MB}
  TYPE=A
  FUSE=0
  BN=32
  BC=32
  BK=32
  VNNI=1
else
  BIN=$1
  ITERS=$2
  MB=$3
  TYPE=$4
  FUSE=$5
  BN=$6
  BC=$7
  BK=$8
  VNNI=$9
fi

if [ "${GREP}" ] && [ "${SORT}" ] && [ "${CUT}" ] && [ "${TR}" ] && [ "${WC}" ]; then
  if [ "$(command -v lscpu)" ]; then
    NS=$(lscpu | ${GREP} -m1 "Socket(s)" | ${TR} -d " " | ${CUT} -d: -f2)
    if [ ! "${NS}" ]; then NS=1; fi
    NC=$((NS*$(lscpu | ${GREP} -m1 "Core(s) per socket" | ${TR} -d " " | ${CUT} -d: -f2)))
    NT=$((NC*$(lscpu | ${GREP} -m1 "Thread(s) per core" | ${TR} -d " " | ${CUT} -d: -f2)))
  elif [ -e /proc/cpuinfo ]; then
    NS=$(${GREP} "physical id" /proc/cpuinfo | ${SORT} -u | ${WC} -l | ${TR} -d " ")
    if [ ! "${NS}" ] || [ ! "${NS}" ]; then NS=1; fi
    NC=$((NS*$(${GREP} -m1 "cpu cores" /proc/cpuinfo | ${TR} -d " " | ${CUT} -d: -f2)))
    NT=$(${GREP} "core id" /proc/cpuinfo  | ${WC} -l | ${TR} -d " ")
  elif [ "Darwin" = "$(uname)" ]; then
    NS=$(sysctl hw.packages    | ${CUT} -d: -f2 | ${TR} -d " ")
    NC=$(sysctl hw.physicalcpu | ${CUT} -d: -f2 | ${TR} -d " ")
    NT=$(sysctl hw.logicalcpu  | ${CUT} -d: -f2 | ${TR} -d " ")
  fi
  if [ "${NC}" ] && [ "${NT}" ]; then
    HT=$((NT/NC))
  else
    NS=1 NC=1 NT=1 HT=1
  fi
  if [ "$(command -v numactl)" ]; then
    NN=$(numactl -H | ${GREP} "available:" | ${CUT} -d' ' -f2)
  else
    NN=${NS}
  fi
fi

CPUFLAGS=$(if [ "${GREP}" ] && [ "${CUT}" ] && [ -e /proc/cpuinfo ]; then ${GREP} -m1 flags /proc/cpuinfo | ${CUT} -d: -f2- || true; fi)
if [ "${GREP}" ] && [ "$(echo "${CPUFLAGS}" | ${GREP} -o avx512er)" ]; then
  if [ "0" != "$((0>NUMA))" ] && [ "0" != "$((NS<NN))" ]; then
    NUMACTL="numactl --preferred=${NS} ${TOOL_COMMAND}"
  elif [ "0" != "$((0<=NUMA && NUMA<NN))" ]; then
    NUMACTL="numactl --preferred=${NUMA} ${TOOL_COMMAND}"
  elif [ "1" != "${NS}" ]; then
    #NUMACTL="numactl -i all ${TOOL_COMMAND}"
    NUMACTL="${TOOL_COMMAND}"
  fi
else
  NUMACTL="${TOOL_COMMAND}"
fi

if [ ! "${OMP_NUM_THREADS}" ] || [ "0" = "${OMP_NUM_THREADS}" ]; then
  if [ "${NUMACTL}" ] && [ ! "${KMP_AFFINITY}" ] && [ ! "${OMP_PROC_BIND}" ]; then
    export KMP_AFFINITY=compact,granularity=fine KMP_HW_SUBSET=1T
  elif [ ! "${OMP_PROC_BIND}" ]; then
    export OMP_PROC_BIND=close OMP_PLACES=threads
  fi
  export OMP_NUM_THREADS=$((NC))
fi

if [ ! "${MB}" ] || [ "0" = "${MB}" ]; then
  MB=${OMP_NUM_THREADS}
fi

if [ ! "${LIBXSMM_TARGET_HIDDEN}" ] || [ "0" = "${LIBXSMM_TARGET_HIDDEN}" ]; then
  echo "OMP_NUM_THREADS=${OMP_NUM_THREADS} OMP_PROC_BIND=\"${OMP_PROC_BIND:-${KMP_AFFINITY}}\" OMP_PROC_BIND NUMACTL=\"${NUMACTL}\""
  echo
fi

if [ "f32" == "${BIN}" ]; then
  PREC=4
elif [ "bf16" == "${BIN}" ]; then
  PREC=2
else
  PREC=1
fi

${NUMACTL} "${HERE}/layer_example" ${ITERS} ${MB} 128 256 ${FUSE} ${TYPE} ${BN} ${BK} ${BC} ${PREC} ${VNNI}
${NUMACTL} "${HERE}/layer_example" ${ITERS} ${MB} 512 1024 ${FUSE} ${TYPE} ${BN} ${BK} ${BC} ${PREC} ${VNNI}
${NUMACTL} "${HERE}/layer_example" ${ITERS} ${MB} 1024 1024 ${FUSE} ${TYPE} ${BN} ${BK} ${BC} ${PREC} ${VNNI}
${NUMACTL} "${HERE}/layer_example" ${ITERS} ${MB} 2048 512 ${FUSE} ${TYPE} ${BN} ${BK} ${BC} ${PREC} ${VNNI}

# ResNet-50 fc layer with bias fusion
${NUMACTL} "${HERE}/layer_example" ${ITERS} ${MB} 2048 1000 1 ${TYPE} ${BN} ${BK} ${BC} ${PREC} ${VNNI}

# post-process logfile (extract and collect performance results)
if [ "${LIBXSMMROOT}" ] && [ -e "${LIBXSMMROOT}/scripts/tool_logrept.sh" ]; then
  ${LIBXSMMROOT}/scripts/tool_logrept.sh
elif [ -e "${HERE}/../../libxsmm/scripts/tool_logrept.sh" ]; then
  ${HERE}/../../libxsmm/scripts/tool_logrept.sh
fi
