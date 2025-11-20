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
    src/main.cpp
    
    src/common.h
    src/common.cpp
    src/application.h
    src/application.cpp
    src/audio.h
    src/audio.cpp
    src/evaluator.h
    src/evaluator.cpp
    src/builtin.h
    src/token.h
    src/expr.h
    src/expr.cpp
    src/bytecode.h
    src/bytecode.cpp
    src/template.h
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT soundtoy)

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

set(CMAKE_BUILD_TYPE Debug)
