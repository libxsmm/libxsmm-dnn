#!/usr/bin/env bash
###############################################################################
# Copyright (c) Intel Corporation - All rights reserved.                      #
# This file is part of the LIBXSMM library.                                   #
#                                                                             #
# For information on the license, see the LICENSE file.                       #
# Further information: https://github.com/libxsmm/libxsmm-dnn/                #
# SPDX-License-Identifier: BSD-3-Clause                                       #
###############################################################################
# Hans Pabst (Intel Corp.)
###############################################################################
# shellcheck disable=SC2012,SC2181

# check if logfile is given (existence, validity is checked later)
if [ ! "${LOGFILE}" ]; then exit 0; fi

# based on https://stackoverflow.com/a/20401674/3001239
flush() {
  if [ "$(command -v sync)" ]; then sync; fi # e.g., async NFS
  if [ "$(command -v script)" ]; then
    script -qefc "$(printf "%q " "$@")" /dev/null
  else
    eval "$@"
  fi
}

# optionally enable script debug
if [ "${DEBUG_REPORT}" ] && [ "0" != "${DEBUG_REPORT}" ]; then
  echo "*** DEBUG ***"
  if [[ ${DEBUG_REPORT} =~ ^[+-]?[0-9]+([.][0-9]+)?$ ]]; then
    set -xv
  else
    set "${DEBUG_REPORT}"
  fi
  PYTHON=$(command -v python3)
  if [ ! "${PYTHON}" ]; then
    PYTHON=$(command -v python)
  fi
  if [ "${PYTHON}" ]; then
    ${PYTHON} -m site --user-site 2>&1 && echo
  fi
  env
  echo "*** DEBUG ***"
fi

# determine LIBXSMM's script directory
if [ "${LIBXSMMROOT}" ] && [ -d "${LIBXSMMROOT}/scripts" ]; then
  SCRDIR=${LIBXSMMROOT}/scripts
else
  HERE=$(cd "$(dirname "$0")" && pwd -P)
  if [ -d "${HERE}/../libxsmm/scripts" ]; then
    SCRDIR=${HERE}/../libxsmm/scripts
  fi
fi

# post-process logfile (extract and collect performance results)
if [ -e "${SCRDIR}/tool_perflog.sh" ]; then
  if flush "${SCRDIR}/tool_perflog.sh" "${LOGFILE}"; then
    LOGDIR=$(cd "$(dirname "${LOGFILE}")" && pwd -P)
  fi
fi

# consider opting-out from generating artifacts
if [ "${ARTOFF}" ] && [ "0" != "${ARTOFF}" ]; then LOGDIR=""; fi

# determine artifact directory (logfile was processed, etc)
if [ "${LOGDIR}" ]; then
  if [ "${ARTDIR}" ] && [ -d "${ARTDIR}" ]; then
    LOGDIR=${ARTDIR}
  else
    if [ "${HOME}" ] && [ -d "${HOME}/artifacts" ]; then
      LOGDIR=${HOME}/artifacts
    elif [ "${HOME_REMOTE}" ] && [ -d "${HOME_REMOTE}/artifacts" ]; then
      LOGDIR=${HOME_REMOTE}/artifacts
    elif [ "$(command -v cut)" ] && [ "$(command -v getent)" ]; then
      ARTUSER=$(ls -g "${LOGFILE}" | cut 2>/dev/null -d' ' -f3) # group
      ARTROOT=$(getent passwd "${ARTUSER}" 2>/dev/null | cut -d: -f6 2>/dev/null)
      if [ ! "${ARTROOT}" ]; then ARTROOT=$(dirname "${HOME}")/${ARTUSER}; fi
      if [ -d "${ARTROOT}/artifacts" ]; then
        LOGDIR=${ARTROOT}/artifacts
      else
        LOGDIR=""
      fi
    fi
  fi
fi

# determine prerequisites for report (artifact directory exists, etc)
if [ "${LOGDIR}" ]; then
  JOBID=${JOBID:-${BUILDKITE_BUILD_NUMBER}}
  STEPNAME=${STEPNAME:-${BUILDKITE_LABEL}}
  if [ "${JOBID}" ] && [ "${STEPNAME}" ]; then
    if [ -e "${LOGDIR}/tool_report.sh" ]; then
      DBSCRT=${LOGDIR}/tool_report.sh
    elif [ -e "${SCRDIR}/tool_report.sh" ]; then
      DBSCRT=${SCRDIR}/tool_report.sh
    fi
  fi
  if [ "${DBSCRT}" ]; then
    PIPELINE=${PIPELINE:-${BUILDKITE_PIPELINE_SLUG}}
    DBMAIN=${PIPELINE:-tool_report}
    DBFILE=${LOGDIR}/${DBMAIN}.json
  else
    LOGDIR=""
  fi
fi

# determine non-default location of weights-file
if [ "${PPID}" ] && [ "$(command -v ps)" ] && [ "$(command -v sed)" ]; then
  TOPEXEC=$(ps -o args= ${PPID})
  WEIGHTS=$(echo "${TOPEXEC}" | sed -n "s/[^[:space:]]*[[:space:]]*\([^.][^.]*\)[.[:space:]]*.*/\1.weights.json/p")
  if [ -e "${WEIGHTS}" ]; then
    DBSCRT="${DBSCRT} -w ${WEIGHTS}"
  fi
fi

# generate report (report script was found, etc)
if [ "${LOGDIR}" ]; then
  mkdir -p "${LOGDIR}/${JOBID}"
  OUTPUT=$(${DBSCRT} -f "${DBFILE}" -g "${LOGDIR}/${JOBID}" \
    -i "${LOGFILE}" -j "${JOBID}" -x -y "${STEPNAME}" -z -v 1)
  RESULT=$?
  if [ "0" = "${RESULT}" ] && [ "${OUTPUT}" ] && \
     [ "$(command -v base64)" ] && \
     [ "$(command -v cut)" ];
  then
    FIGURE=$(echo "${OUTPUT}" | cut -d' ' -f1)
    if [ -e "${FIGURE}" ]; then
      FIGURE=$(base64 -w0 "${FIGURE}")
      RESULT=$?
      if [ "0" = "${RESULT}" ] && [ "${FIGURE}" ]; then
        printf "\n\033]1338;url=\"data:image/png;base64,%s\";alt=\"%s\"\a\n" \
          "${FIGURE}" "${STEPNAME}"
      fi
    fi
  fi
fi
