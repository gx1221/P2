cmake_minimum_required(VERSION 3.10)
project(NES_Emulator CXX)


# C++ standard and warnings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -g")

# --- Find SDL2 ---
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# --- Source files ---
set(SOURCES
    src/cpu.cpp
    src/mapper.cpp
    src/ppu.cpp
    src/apu.cpp
    src/input.cpp
)

# --- Main targets ---
add_executable(test src/test.cpp ${SOURCES})
target_link_libraries(test ${SDL2_LIBRARIES})

# --- Custom run targets ---
add_custom_target(run
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test
    DEPENDS test
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Set working directory to project root when running from CMake
set_target_properties(test PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
