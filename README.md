# Soundtoy

This is a small demo project about generating audio from mathematical expressions.
The user provides an expression that will be evaluated repeatedly to produce audio samples.

The expression parsing and sample generation code is written in C++ and it is under the st/ folder.
There is a desktop demo under the desktop/ folder and an unfinished web demo under the web/ folder.

Desktop version uses SDL alongside with the companion libraries of SDL_ttf, SDL_image and SDL_mixer.
They are included as git submodules under the vendor/ folder.
Desktop version uses cmake for building.

## Building

The desktop version is a standard cmake project and you can generate build files with:
```bash
mkdir build
cd build
cmake ..
```
You also want to make sure you have at least one audio backend for your platform is included in the build of SDL.

The application will search for assets in the folder the executable is located in.
So you might have to copy the asset/ folder at the top level of the repository to where the executable is located in to get it working.
By default the executable will go into build/bin/.
