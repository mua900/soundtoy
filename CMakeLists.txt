cmake_minimum_required(VERSION 3.10)

project(soundtoy)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# For multi-config generators (Visual Studio, Xcode)
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin/${CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib/${CONFIG})
endforeach()

set(CMAKE_CXX_STANDARD 20)

add_executable(soundtoy
    desktop/main.cpp

    desktop/application.h
    desktop/application.cpp
    desktop/audio.h
    desktop/audio.cpp
    desktop/ui.h
    desktop/ui.cpp
    desktop/spectogram.h
    desktop/spectogram.cpp
)

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
    st/dsp/fourier.h
    st/dsp/fourier.cpp
)

target_include_directories(soundtoy_lib PRIVATE st/common)

target_include_directories(soundtoy PRIVATE st/common)
target_include_directories(soundtoy PRIVATE st/evaluation)
target_include_directories(soundtoy PRIVATE st/dsp)

target_link_libraries(soundtoy PRIVATE soundtoy_lib)


# SDL
add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

set(SDLIMAGE_VENDORED ON)
set(SDLTTF_VENDORED ON)

# SDL_image
# turn off unused formats
set(SDLIMAGE_ANI  OFF)
set(SDLIMAGE_AVIF OFF)
set(SDLIMAGE_BMP  OFF)
set(SDLIMAGE_GIF  OFF)
set(SDLIMAGE_JPG  OFF)
set(SDLIMAGE_JXL  OFF)
set(SDLIMAGE_LBM  OFF)
set(SDLIMAGE_PCX  OFF)
set(SDLIMAGE_PNM  OFF)
set(SDLIMAGE_QOI  OFF)
set(SDLIMAGE_SVG  OFF)
set(SDLIMAGE_TGA  OFF)
set(SDLIMAGE_TIF  OFF)
set(SDLIMAGE_WEBP OFF)
set(SDLIMAGE_XCF  OFF)
set(SDLIMAGE_XPM  OFF)
set(SDLIMAGE_XV   OFF)
# png
set(SDLIMAGE_PNG ON)

add_subdirectory(vendor/SDL_image EXCLUDE_FROM_ALL)

# SDL_mixer
# add_subdirectory(vendor/SDL_mixer EXCLUDE_FROM_ALL)

# SDL_ttf
add_subdirectory(vendor/SDL_ttf EXCLUDE_FROM_ALL)

target_include_directories(soundtoy PRIVATE vendor/SDL/include/)
target_include_directories(soundtoy PRIVATE vendor/SDL_image/include/)
# target_include_directories(soundtoy PRIVATE vendor/SDL_mixer/include)
target_include_directories(soundtoy PRIVATE vendor/SDL_ttf/include/)

target_link_libraries(
    soundtoy
    PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
#     SDL3_mixer::SDL3_mixer
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
