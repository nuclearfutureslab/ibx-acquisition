cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(acquisition)

#####
# ARM SETTINGS
#####
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(FLOAT_ABI_SUFFIX "")
if (NOT SOFTFP)
  set(FLOAT_ABI_SUFFIX "hf")
endif()

# Compilers
INCLUDE(CMakeForceCompiler)

# find_program(CMAKE_C_COMPILER NAMES arm-linux-gnueabi${FLOAT_ABI_SUFFIX}-gcc)
# find_program(CMAKE_CXX_COMPILER NAMES arm-linux-gnueabi${FLOAT_ABI_SUFFIX}-g++)
set(ARM_LINUX_SYSROOT /usr/arm-linux-gnueabi${FLOAT_ABI_SUFFIX} CACHE PATH "ARM cross compilation system root")

SET(CMAKE_C_COMPILER   arm-linux-gnueabihf-gcc)
# 4.9 seems to be the max version for the RedPitaya debian
SET(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} ${ARM_LINUX_SYSROOT})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)

#####
# ARM SETTINGS END
#####

SET(CMAKE_CXX_FLAGS "-std=c++0x")

# Sources and Headers of Project
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

# Include Directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/pitaya)

# Executable
add_executable(acquisition acquisition.cc ${sources} ${headers})
