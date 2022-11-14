#!/usr/bin/env bash
###############################################################################
# Copyright (c) Intel Corporation - All rights reserved.                      #
# This file is part of the LIBXSMM library.                                   #
#                                                                             #
# For information on the license, see the LICENSE file.                       #
# Further information: https://github.com/libxsmm/libxsmm/                    #
# SPDX-License-Identifier: BSD-3-Clause                                       #
###############################################################################
# Hans Pabst (Intel Corp.)
###############################################################################
# shellcheck disable=SC2207

DATAMASH=$(command -v datamash)
PASTE=$(command -v paste)
SED=$(command -v sed)
BC=$(command -v bc)
SEP=";"

if [ ! "${DATAMASH}" ]; then
  >&2 echo "ERROR: missing GNU Datamash!"
  exit 1
fi
if [ ! "${BC}" ]; then
  >&2 echo "ERROR: missing Basic Calculator!"
  exit 1
fi
if [ ! "${PASTE}" ] || [ ! "${SED}" ]; then
  >&2 echo "ERROR: missing prerequisites!"
  exit 1
fi
# ensure proper permissions
if [ "${UMASK}" ]; then
  UMASK_CMD="umask ${UMASK};"
  eval "${UMASK_CMD}"
fi

ARGC=$#
while test $# -gt 0; do
  case "$1" in
  -h|--help)
    HELP="echo"
    shift $#;;
  -o|--out)
    OFILE=$2
    shift 2;;
  -s|--sep)
    SEP=$2
    shift 2;;
  -q|--sum)
    SUM=1
    shift 1;;
  *)
    if [ ! "${IFILE}" ]; then IFILE=$1; fi
    shift 1;;
  esac
done
if [ "${IFILE}" ] && [ "${IFILE}" ] && \
   [ "$(cd "$(dirname "${IFILE}")" && pwd)/$(basename "${IFILE}")" = \
     "$(cd "$(dirname "${OFILE}")" && pwd)/$(basename "${OFILE}")" ];
then
  >&2 echo "ERROR: infile and outfile are equal!"
  exit 1
fi

if [ "${HELP}" ] || [ "0" = "${ARGC}" ]; then
  if [ "0" = "${ARGC}" ]; then HELP=">&2 echo"; fi
  eval "${HELP} \"Usage: $0 [options] IFILE\""
  eval "${HELP} \"       -o|--out file: name of CSV-file\""
  eval "${HELP} \"       -s|--sep char: delimiter (CSV)\""
  eval "${HELP} \"       -q|--sum: show summary only\""
  eval "${HELP}"
  eval "${HELP} \"Example: ./run_convs.sh 1 10 -1 f32 F 0 0 64 64 1 \\\\\""
  eval "${HELP} \"         | $0 -o performance.csv /dev/stdin\""
  eval "${HELP}"
  if [ "0" = "${ARGC}" ]; then exit 1; else exit 0; fi
fi

# handle errors more strictly
set -eo pipefail

if [ ! "${OFILE}" ]; then
  OFILE=$(mktemp)
  trap 'rm -f ${OFILE}' EXIT
fi

if [ "${SLURM_JOB_PARTITION}" ]; then
  echo "+++ PERFORMANCE ${SLURM_JOB_PARTITION}"
else
  echo "+++ PERFORMANCE ${HOSTNAME}"
fi

echo "FLOPS${SEP}TIME" >"${OFILE}"
PATTERN="[[:space:]]*=[[:space:]]*\(..*\)/\1/p"
${SED} -n "s/^GFLOP${PATTERN};s/^fp time${PATTERN}" "${IFILE}" 2>/dev/null \
  | ${SED} "s/\r//g" | ${PASTE} -d"${SEP}" - - >>"${OFILE}" 2>/dev/null

NUMPAT="s/\([+-]\{0,1\}[0-9]*\.\{0,1\}[0-9]\{1,\}\)[eE]+\{0,1\}\(-\{0,1\}\)\([0-9]\{1,\}\)/(\1*10^\2\3)/g"
if [ ! "${SUM}" ] || [ "0" = "${SUM}" ]; then
  LAYER=0
  while read -r LINE; do
    if [ "0" != "${LAYER}" ]; then # skip header line
      RESULT=($(echo "${LINE}" | ${SED} 2>/dev/null -e "${NUMPAT}" -e "s/;/ /"))
      if [ "${RESULT[0]}" ] && [ "${RESULT[1]}" ]; then
        printf "Layer %i: %.0f GFLOPS/s\n" "${LAYER}" "$(${BC} 2>/dev/null -l <<<"${RESULT[0]}/${RESULT[1]}")"
      fi
    fi
    LAYER=$((LAYER+1))
  done < "${OFILE}"
fi

RESULT=($(${DATAMASH} 2>/dev/null <"${OFILE}" --header-in -t"${SEP}" --output-delimiter=" " sum 1 sum 2 \
  | ${SED} 2>/dev/null "${NUMPAT}"))
if [ "${RESULT[0]}" ] && [ "${RESULT[1]}" ]; then
  printf "%f ms\n" "$(${BC} 2>/dev/null -l <<<"1000*${RESULT[1]}")"
  printf "%.0f GFLOPS/s\n" "$(${BC} 2>/dev/null -l <<<"${RESULT[0]}/${RESULT[1]}")"
  printf "%.0f Hz (fps)\n" "$(${BC} 2>/dev/null -l <<<"1/${RESULT[1]}")"
fi
