#include <framework/Application.h>

int main()
{
    Application app;

    if (!app.Initialize())
    {
        return -1;
    }

    while (app.IsRunning())
    {
        app.BeginFrame();

        app.EndFrame();
    }

    return 0;
}