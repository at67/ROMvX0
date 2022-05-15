cmake_minimum_required(VERSION 3.7)

project(gtbasic)

set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_CONFIGURATION_TYPES "Release;Debug" CACHE STRING "" FORCE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_BUILD_TYPE "Release")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Clang or AppleClang
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-char-subscripts")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-dollar-in-identifier-extension")
endif()

# Pi4 and Pi400
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
    set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -latomic")
    add_compile_options(-mcpu=cortex-a72 -mfloat-abi=hard -mfpu=neon-fp-armv8)
endif()

file(GLOB headers *.h)
file(GLOB sources *.cpp)
set(headers ${headers})
set(sources ${sources})

add_definitions(-DSTAND_ALONE)

if(MSVC)
    add_compile_options(/W4)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
elseif(MINGW)
    add_compile_options(-Wall -Wextra -pedantic)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()
add_executable(gtbasic ${headers} ${sources})
    
target_link_libraries(gtbasic)