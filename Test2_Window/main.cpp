// Test for creation of a window
// Renders animated colors through the color buffer clear color

#include <window/window.h>

#ifdef TEST_WIN
#include <GLFW/glfw3.h>
#else // TEST_GX2
#include <gx2/clear.h>
#endif

int main()
{
    u32 fb_width, fb_height;
    if (!WindowInit(1280, 720, &fb_width, &fb_height))
        return -1;

    // Make window context current
    //WindowMakeContextCurrent();
    // No need, automatically done by WindowInit()

    f32 r = 0.0, r_step = 0.01;
    f32 g = 0.0, g_step = 0.02;
    f32 b = 0.0, b_step = 0.04;

    while (WindowIsRunning())
    {
        // Window context should already be current at this point

#ifdef TEST_WIN

        // Set the current clear color to the given color
        glClearColor(r, g, b, 1.0f);

        // Clear the current color buffer
        glClear(GL_COLOR_BUFFER_BIT);

#else // TEST_GX2

        // GX2 does not provide any function to clear the current color buffer,
        // nor a function to get the current color buffer

        // Clear the window color buffer explicitly with the given color
        // Does not need a current context to be set
        GX2ClearColor(WindowGetColorBuffer(), r, g, b, 1.0f);

        // GX2ClearColor invalidates the current context and the window context
        // must be made current again
        WindowMakeContextCurrent();

#endif
        r += r_step;
        g += g_step;
        b += b_step;

        if (r >= 1.0 || r <= 0.0)
        {
            r_step = -r_step;

            if (r > 1.0)
                r = 1.0;
            else if (r < 0.0)
                r = 0.0;
        }

        if (g >= 1.0 || g <= 0.0)
        {
            g_step = -g_step;

            if (g > 1.0)
                g = 1.0;
            else if (g < 0.0)
                g = 0.0;
        }

        if (b >= 1.0 || b <= 0.0)
        {
            b_step = -b_step;

            if (b > 1.0)
                b = 1.0;
            else if (b < 0.0)
                b = 0.0;
        }

        WindowSwapBuffers();
    }

    WindowExit();
    return 0;
}
