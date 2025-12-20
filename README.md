# Soundtoy

This is a small demo project about generating audio from mathematical expressions.
The user provides an expression that will be evaluated repeatedly to produce audio samples.

The expression parsing and sample generation logic is in a seperate library different platform implementations can use under the st/ folder.
There is a desktop demo under the desktop/ folder and web version under the web/ folder.

Desktop version uses SDL and the companion libraries of SDL_ttf and SDL_image. They are included as git submodules under the vendor/ folder.
Desktop version uses cmake for building.

If you want to build the desktop version you need to have cmake and a suitable C++ compiler.
You also want to make sure you have at least one audio backend appropriate to your platform is included in the build of SDL.

There is also the small problem of the application searching for assets in the folder the executable is located in.
So you might have to copy the asset/ folder at the top level to build/bin/ to get it working.
