#include <iostream>

#include "application.h"

int main()
{
    Application app;

    if (!app.initialize())
        return 1;

    while (!app.m_quit)
    {
        app.handle_events();
        app.update();
        app.draw();
    }

    SDL_Quit();

    return 0;
}
