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

set(CMAKE_CXX_STANDARD 20)

add_executable(soundtoy
    desktop/main.cpp
    
    desktop/application.h
    desktop/application.cpp
    desktop/audio.h
    desktop/audio.cpp
    desktop/ui.h
    desktop/spectogram.h

    vendor/imgui/imgui.cpp
    vendor/imgui/imgui.h
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_demo.cpp

    # the backends we use
    vendor/imgui/backends/imgui_impl_sdl3.cpp
    vendor/imgui/backends/imgui_impl_sdlrenderer3.cpp
    vendor/imgui/backends/imgui_impl_sdl3.h
    vendor/imgui/backends/imgui_impl_sdlrenderer3.h
)

target_include_directories(soundtoy PRIVATE vendor/imgui)
target_include_directories(soundtoy PRIVATE vendor/imgui/backends)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT soundtoy)

add_library(soundtoy_lib STATIC
    # common
    st/common/common.h
    st/common/common.cpp
    st/common/template.h

    # evaluation
    st/evaluation/evaluator.h
    st/evaluation/evaluator.cpp
    st/evaluation/builtin.h
    st/evaluation/builtin.cpp
    st/evaluation/token.h
    st/evaluation/expr.h
    st/evaluation/expr.cpp
    st/evaluation/bytecode.h
    st/evaluation/bytecode.cpp
    st/evaluation/api.h
    st/evaluation/api.cpp

    # dsp
    st/dsp/fft.h
    st/dsp/fft.cpp
)

target_include_directories(soundtoy_lib PRIVATE st/common)

target_include_directories(soundtoy PRIVATE st/common)
target_include_directories(soundtoy PRIVATE st/evaluation)
target_include_directories(soundtoy PRIVATE st/dsp)

target_link_libraries(soundtoy PRIVATE soundtoy_lib)


# SDL
add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

# SDL_image
set(SDLIMAGE_VENDORED ON)
# turn off unused formats
set(SDLIMAGE_AVIF OFF)
set(SDLIMAGE_BMP OFF)
set(SDLIMAGE_JPEG OFF)
set(SDLIMAGE_WEBP OFF)
add_subdirectory(vendor/SDL_image EXCLUDE_FROM_ALL)

# SDL_ttf
set(SDLTTF_VENDORED ON)
add_subdirectory(vendor/SDL_ttf EXCLUDE_FROM_ALL)

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

include(GNUInstallDirs)

install(TARGETS soundtoy
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY asset
	DESTINATION ${CMAKE_INSTALL_DATADIR}/flying-carpet/soundtoy
)

option(INSTALL_VENDORED_SDL "Install vendored SDL libraries" OFF)

if (INSTALL_VENDORED_SDL)
	get_target_property(SDL3_TARGET SDL3::SDL3 ALIASED_TARGET)
	get_target_property(SDL3_IMAGE_TARGET SDL3_image::SDL3_image ALIASED_TARGET)
	get_target_property(SDL3_TTF_TARGET SDL3_ttf::SDL3_ttf ALIASED_TARGET)

	install(
		TARGETS
			${SDL3_TARGET}
			${SDL3_IMAGE_TARGET}
			${SDL3_TTF_TARGET}

			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
endif()
