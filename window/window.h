// Windowing library built on GX2 with basic operations inspired by glfw

#ifndef WINDOW_H_
#define WINDOW_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <test_types.h>

// Initialize the window
// Parameters:
// - width: The desired width.
// - height: The desired height.
// Outputs:
// - pWidth: The actual width set for the framebuffer (optional)
//           It may be the input "width" or a more optimal value
// - pHeight: The actual height set for the framebuffer (optional)
//            It may be the input "height" or a more optimal value
bool WindowInit(u32 width, u32 height, u32* pWidth, u32* pHeight);

// Make the context of this window the current context
void WindowMakeContextCurrent();

// Set the swap interval (how many refreshes to wait before flipping the scan buffers)
// Parameters:
// - swap_interval: The swap interval is this value divided by the refresh rate (59.94 Hz on Wii U)
//                  e.g. a value of 2 will give a swap interval of 2 / 59.94 = ~33 ms on Wii U
//                  A value of 0 means swapping should happen as quickly as possible (For GLFW)
void WindowSetSwapInterval(u32 swap_interval);

// Function to determine whether the program should continue running or exit
bool WindowIsRunning();

// Swap the front and back buffers
// This function will perform a GPU flush and block until swapping is done
// For Wii U, TV output is automatically duplicated to the Gamepad
void WindowSwapBuffers();

// Function to be called by user at application exit to free resources allocated by this library
void WindowExit();

#ifdef TEST_GX2

#include <gx2/surface.h>

GX2ColorBuffer* WindowGetColorBuffer();
GX2DepthBuffer* WindowGetDepthBuffer();

#endif // TEST_GX2

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // WINDOW_H_
