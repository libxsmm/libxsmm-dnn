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

if [ "${LOGFILE}" ]; then
  HERE=$(cd "$(dirname "$0")" && pwd -P)

  # flush, e.g., asynchronous NFS mount
  if [ "$(command -v sync)" ]; then sync; fi 
  if [ "${LIBXSMMROOT}" ] && [ -e "${LIBXSMMROOT}/scripts/tool_perflog.sh" ]; then
    "${LIBXSMMROOT}/scripts/tool_perflog.sh" "${LOGFILE}"
  elif [ -e "${HERE}/../libxsmm/scripts/tool_perflog.sh" ]; then
    "${HERE}/../libxsmm/scripts/tool_perflog.sh" "${LOGFILE}"
  fi
fi
