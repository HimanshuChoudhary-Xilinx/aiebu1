# SPDX-License-Identifier: MIT
# Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.
if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

# Use static Boost libraries
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME OFF)

# We use header only libraries and regex component
if(${CMAKE_VERSION} VERSION_LESS "3.30")
  find_package(Boost REQUIRED COMPONENTS regex)
else()
  find_package(Boost CONFIG REQUIRED COMPONENTS regex)
endif()

message("-- Boost version: ${Boost_VERSION}")
message("-- Boost include dir:${Boost_INCLUDE_DIRS}")
message("-- Boost libraries:${Boost_LIBRARIES}")

# Some later versions of boost spews warnings form property_tree
# but can be disabled with this setting
add_compile_options("-DBOOST_BIND_GLOBAL_PLACEHOLDERS")
include_directories(${Boost_INCLUDE_DIRS})
