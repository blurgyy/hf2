cmake_minimum_required(VERSION 3.19)
project("hf2") # Project name

set(target_name "hf2") # Target name (target can be an executable or a library)
set(sources
    src/main.cpp
    # Sources
)

# # Enable OpenMP
# set(CMAKE_CXX_FLAGS "-fopenmp ${CMAKE_CXX_FLAGS}")

set(CMAKE_C_FLAGS_DEBUG "-Og -g -Wall")
set(CMAKE_C_FLAGS_RELEASE "-O2 -w -DNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -w -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -Wall -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")

# # Set this in Makefile and use `make debug|release` instead.
# # Default target in Makefile should be set to debug.
# set(CMAKE_BUILD_TYPE "Debug")
# set(CMAKE_BUILD_TYPE "Release")

# Build an executable:
add_executable(${target_name} "${sources}")
# # Or, build a library to be linked:
# add_library(${target_name} STATIC ${sources})
# add_library(${target_name} SHARED ${sources})
# add_library(${target_name} MODULE ${sources})  # I have no idea what this is

# # Eigen3
# find_package(Eigen3)
# include_directories(${EIGEN3_INCLUDE_DIRS})

# fmt
target_link_libraries(${target_name} fmt)

# # CGAL, build with `-DCGAL_HEADER_ONLY=OFF` to build shared objects for
# # dynamic linking.
# find_package(CGAL REQUIRED)
# message(STATUS ${CGAL_INCLUDE_DIRS})
# include_directories(${CGAL_INCLUDE_DIRS})
# target_link_libraries(${target_name} ${CGAL_LIBRARIES})
# # gmp and mpfr (for CGAL projects)
# target_link_libraries(${target_name} gmp mpfr)

# # PCL
# find_package(PCL REQUIRED COMPONENTS common io)
# include_directories(${PCL_INCLUDE_DIRS})
# link_directories(${PCL_LIBRARY_DIRS})
# add_definitions(${PCL_DEFINITIONS})
# target_link_libraries(${target_name} ${PCL_LIBRARIES})

# # OpenCV
# find_package(OpenCV)
# include_directories(${OpenCV_INCLUDE_DIRS})
# target_link_libraries(${target_name} ${OpenCV_LIBS})

# # Magick++ (ImageMagick)
# find_package(ImageMagick COMPONENTS Magick++)
# include_directories(${ImageMagick_INCLUDE_DIRS})
# target_link_libraries(${target_name}
# ${ImageMagick_LIBRARIES}
# MagickCore-7.Q16HDRI # Use this jointly with -DMAGICKCORE_QUANTUM_DEPTH=16.
# )
# add_definitions(-DMAGICKCORE_QUANTUM_DEPTH=16)
# add_definitions(-DMAGICKCORE_HDRI_ENABLE=1)

# External libraries
include_directories("extern")

# # Custom headers, add this after all other dependencies
# include_directories("include")

# # Link libraries, be cautious about the linking order.
# # **This builds library 'wheels'**
# add_subdirectory("include")
# # Link custom library from "include/"
# target_link_libraries(${target_name} wheels)
# # **This builds library 'external'**
# add_subdirectory("extern")
# # Link external library from "extern/"
# target_link_libraries(${target_name} external)

add_subdirectory(extern/tinyobjloader)
target_link_libraries(${target_name} tinyobjloader)

# vim: set ft=cmake:

# Author: Blurgy <gy@blurgy.xyz>
# Date:   May 21 2021, 10:49 [CST]
