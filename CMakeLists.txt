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

# TODO ifdef this to be excluded on builds for other platforms
add_executable(soundtoy
    desktop/main.cpp
    
    desktop/application.h
    desktop/application.cpp
    desktop/audio.h
    desktop/audio.cpp

    vendor/imgui/imgui.cpp
    vendor/imgui/imgui.h
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_demo.cpp
)

target_include_directories(soundtoy PRIVATE vendor/imgui)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT soundtoy)

add_library(soundtoy_lib STATIC
    st/common.h
    st/common.cpp
    st/evaluator.h
    st/evaluator.cpp
    st/builtin.h
    st/builtin.cpp
    st/token.h
    st/expr.h
    st/expr.cpp
    st/bytecode.h
    st/bytecode.cpp
    st/template.h

    st/api.h
    st/api.cpp
)

target_include_directories(soundtoy PRIVATE st)
target_link_libraries(soundtoy PRIVATE soundtoy_lib)


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
