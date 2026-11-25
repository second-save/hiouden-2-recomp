cmake_minimum_required(VERSION 3.16)
project(hiouden2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
find_package(SDL2 REQUIRED)

add_executable(hiouden2
    src/main.cpp
    src/cpu/cpu386.cpp
    src/hw/ega_vga.cpp
    src/hw/audio.cpp
    src/hw/dos_runtime.cpp
)

target_include_directories(hiouden2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
)

if(TARGET SDL2::SDL2)
    target_link_libraries(hiouden2 PRIVATE SDL2::SDL2)
else()
    target_link_libraries(hiouden2 PRIVATE ${SDL2_LIBRARIES})
endif()

target_compile_options(hiouden2 PRIVATE -Wall -Wextra -Wno-unused-parameter)
