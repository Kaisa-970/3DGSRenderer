#include "App/AppDemo.h"
#include <stdexcept>
#ifdef GSRENDERER_OS_WINDOWS
#include <windows.h>
#endif

const int WIN_WIDTH = 1920;
const int WIN_HEIGHT = 1080;

int main(int argc, char *argv[])
{
#ifdef GSRENDERER_OS_WINDOWS
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    try
    {
        AppDemo app({WIN_WIDTH, WIN_HEIGHT, "3DGS Engine"});
        app.Init();
        app.Run();
        app.Shutdown();

        return 0;
    }
    catch (const std::exception &e)
    {
        // LOG_ERROR("发生错误: {}", e.what());
        return -1;
    }
}
