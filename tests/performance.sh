#!/usr/bin/env bash
###############################################################################
# Copyright (c) Intel Corporation - All rights reserved.                      #
# This file is part of the LIBXSMM library.                                   #
#                                                                             #
# For information on the license, see the LICENSE file.                       #
# Further information: https://github.com/libxsmm/libxsmm/                    #
# SPDX-License-Identifier: BSD-3-Clause                                       #
###############################################################################
# shellcheck disable=SC2207

DATAMASH=$(command -v datamash)
PASTE=$(command -v paste)
SED=$(command -v sed)
BC=$(command -v bc)
SEP=";"

if [ ! "${DATAMASH}" ] || [ ! "${PASTE}" ] || [ ! "${SED}" ] || [ ! "${BC}" ]; then
  >&2 echo "ERROR: missing prerequisites!"
  exit 1
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
  eval "${HELP}"
  eval "${HELP} \"Example: ./run_convs.sh 1 10 -1 f32 F 0 0 64 64 1 \\\\\""
  eval "${HELP} \"         | $0 -o performance.csv /dev/stdin\""
  eval "${HELP}"
  if [ "0" = "${ARGC}" ]; then exit 1; else exit 0; fi
fi

if [ ! "${OFILE}" ]; then
  OFILE=$(mktemp)
  trap 'rm -f ${OFILE}' EXIT
fi

echo "FLOPS${SEP}TIME" >"${OFILE}"
PATTERN="[[:space:]]*=[[:space:]]*\(..*\)/\1/p"
${SED} -n "s/^GFLOP${PATTERN};s/^fp time${PATTERN}" "${IFILE}" \
  | ${SED} "s/\r//g" | ${PASTE} -d"${SEP}" - - >>"${OFILE}"

RESULT=($(${DATAMASH} <"${OFILE}" --header-in -t"${SEP}" --output-delimiter=" " sum 1 sum 2))
printf "%.1f GFLOPS/s\n" "$(${BC} -l <<<"${RESULT[0]}/${RESULT[1]}")"
printf "%.1f Hz (fps)\n" "$(${BC} -l <<<"1/${RESULT[1]}")"