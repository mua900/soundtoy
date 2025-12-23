# Soundtoy

This is a small demo project about generating audio from mathematical expressions.
The user provides an expression that will be evaluated repeatedly to produce audio samples.

The expression parsing and sample generation code is written in C++ and is shared between different platform implementations. It is under the st/ folder.
There is a desktop demo under the desktop/ folder and a web version under the web/ folder.

Desktop version uses SDL and the companion libraries of SDL_ttf and SDL_image. They are included as git submodules under the vendor/ folder.
Desktop version uses cmake for building.

The desktop version is a standard cmake project and you can generate the build files for your compiler with:
```bash
mkdir build
cd build
cmake ..
```
You also want to make sure you have at least one audio backend for your platform is included in the build of SDL.

There is also the problem that the application is searching for assets in the folder the executable is located in.
So you might have to copy the asset/ folder at the top level of the repository to where the executable is located in to get it working.
By default the executable will go into build/bin/.
