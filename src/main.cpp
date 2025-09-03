#include <iostream>

#include "application.h"

int main()
{
    Application app;
    if (!initialize(&app)) return 1;

    while (!app.quit)
    {
        app.handle_events();
        app.update();
        app.draw();
    }

    SDL_Quit();

    return 0;
}
