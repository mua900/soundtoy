cmake_minimum_required(VERSION 3.10)

project(soundtoy)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# For multi-config generators (Visual Studio, Xcode)
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib)
endforeach()

add_executable(soundtoy
    desktop/main.cpp
    
    desktop/application.h
    desktop/application.cpp
    desktop/audio.h
    desktop/audio.cpp
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT soundtoy)

add_library(core STATIC
    core/common.h
    core/common.cpp
    core/evaluator.h
    core/evaluator.cpp
    core/builtin.h
    core/builtin.cpp
    core/token.h
    core/expr.h
    core/expr.cpp
    core/bytecode.h
    core/bytecode.cpp
    core/template.h

    core/api.h
    core/api.cpp
)

target_include_directories(soundtoy PRIVATE core)
target_link_libraries(soundtoy PRIVATE core)


# SDL
add_subdirectory(vendor/SDL)
add_subdirectory(vendor/SDL_image)
add_subdirectory(vendor/SDL_ttf)

target_include_directories(soundtoy PRIVATE vendor/SDL/include/)
target_include_directories(soundtoy PRIVATE vendor/SDL_image/include/)
target_include_directories(soundtoy PRIVATE vendor/SDL_ttf/include/)

target_link_libraries(
    soundtoy
    PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
)
