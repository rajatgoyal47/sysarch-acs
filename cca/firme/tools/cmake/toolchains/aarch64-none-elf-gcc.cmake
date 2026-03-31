#-------------------------------------------------------------------------------
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Prefer a CROSS_COMPILE prefix (like aarch64-none-elf-) if provided.
if(DEFINED ENV{CROSS_COMPILE} AND NOT "$ENV{CROSS_COMPILE}" STREQUAL "")
  set(_CROSS "$ENV{CROSS_COMPILE}")
else()
  set(_CROSS "aarch64-none-elf-")
endif()

set(CMAKE_C_COMPILER   "${_CROSS}gcc")
set(CMAKE_ASM_COMPILER "${_CROSS}gcc")
set(CMAKE_AR           "${_CROSS}ar")
set(CMAKE_RANLIB       "${_CROSS}ranlib")
set(CMAKE_OBJCOPY      "${_CROSS}objcopy")
set(CMAKE_OBJDUMP      "${_CROSS}objdump")
