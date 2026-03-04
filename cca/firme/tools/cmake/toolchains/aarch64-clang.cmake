#-------------------------------------------------------------------------------
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Cross-compile with Clang. Partners may need to provide a sysroot/toolchain
# depending on their environment.
set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET aarch64-linux-gnu)
set(CMAKE_ASM_COMPILER_TARGET aarch64-linux-gnu)

# Prefer a CROSS_COMPILE prefix (like aarch64-linux-gnu-) if provided for binutils.
if(DEFINED ENV{CROSS_COMPILE} AND NOT "$ENV{CROSS_COMPILE}" STREQUAL "")
  set(_CROSS "$ENV{CROSS_COMPILE}")
else()
  set(_CROSS "aarch64-linux-gnu-")
endif()

# Objcopy/objdump are required by the build. Prefer LLVM tools when present,
# otherwise fall back to GNU binutils from CROSS_COMPILE.
find_program(_LLVM_OBJCOPY llvm-objcopy)
find_program(_LLVM_OBJDUMP llvm-objdump)

if(_LLVM_OBJCOPY)
  set(CMAKE_OBJCOPY "${_LLVM_OBJCOPY}")
else()
  set(CMAKE_OBJCOPY "${_CROSS}objcopy")
endif()

if(_LLVM_OBJDUMP)
  set(CMAKE_OBJDUMP "${_LLVM_OBJDUMP}")
else()
  set(CMAKE_OBJDUMP "${_CROSS}objdump")
endif()

set(CMAKE_AR     "${_CROSS}ar")
set(CMAKE_RANLIB "${_CROSS}ranlib")
