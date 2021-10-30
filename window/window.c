// Windowing library built on GX2 with basic operations inspired by glfw

#include "window.h"

#ifdef TEST_WIN

#include <GLFW/glfw3.h>

static GLFWwindow* gWindowHandleWin = NULL;

#else // TEST_GX2

#include <coreinit/memdefaultheap.h>
#include <gx2/context.h>
#include <gx2/display.h>
#include <gx2/event.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/state.h>
#include <gx2/swap.h>

static void* gCmdlist = NULL;
static GX2ContextState* gContext = NULL;
static void* gTvScanBuffer = NULL;
static void* gDrcScanBuffer = NULL;
static GX2ColorBuffer gColorBuffer;
static void* gColorBufferImageData = NULL;
static GX2DepthBuffer gDepthBuffer;
static void* gDepthBufferImageData = NULL;

#endif

static bool gInitialized = false;

bool WindowInit(u32 width, u32 height, u32* pWidth, u32* pHeight)
{
    // Prevent re-initialization
    if (gInitialized)
        return false;

#ifdef TEST_WIN

    // Initialize GLFW
    if (!glfwInit())
        return false;

    // Disable resizing
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Assume double-buffering is already on

    // Create the window instance
    gWindowHandleWin = glfwCreateWindow(width, height, "TEST", NULL, NULL);
    if (!gWindowHandleWin)
    {
        WindowExit();
        return false;
    }

    if (pWidth || pHeight)
    {
        // Get the framebuffer width and height
        int fb_width, fb_height;
        glfwGetFramebufferSize(gWindowHandleWin, &fb_width, &fb_height);

        if (pWidth)
            *pWidth = fb_width;
        if (pHeight)
            *pHeight = fb_height;
    }

    // Make context of window current
    WindowMakeContextCurrent();

#else // TEST_GX2

    // Allocate GX2 command buffer
    gCmdlist = MEMAllocFromDefaultHeapEx(
        0x400000,                    // A very commonly used size in Nintendo games
        GX2_COMMAND_BUFFER_ALIGNMENT // Required alignment
    );

    if (!gCmdlist)
        return false;

    // Several parameters to initialize GX2 with
    u32 initAttribs[] = {
        GX2_INIT_CMD_BUF_BASE, (uintptr_t)gCmdlist, // Command Buffer Base Address
        GX2_INIT_CMD_BUF_POOL_SIZE, 0x400000,       // Command Buffer Size
        GX2_INIT_ARGC, 0,                           // main() arguments count
        GX2_INIT_ARGV, (uintptr_t)NULL,             // main() arguments vector
        GX2_INIT_END                                // Ending delimiter
    };

    // Initialize GX2
    GX2Init(initAttribs);

    u32 fb_width, fb_height;
    u32 drc_width, drc_height;

    // Allocate TV scan buffer
    {
        GX2TVRenderMode tv_render_mode;

        // Get current TV scan mode
        GX2TVScanMode tv_scan_mode = GX2GetSystemTVScanMode();

        // Determine TV framebuffer dimensions (scan buffer, specifically)
        if (tv_scan_mode != GX2_TV_SCAN_MODE_576I && tv_scan_mode != GX2_TV_SCAN_MODE_480I
            && width >= 1920 && height >= 1080)
        {
            fb_width = 1920;
            fb_height = 1080;
            tv_render_mode = GX2_TV_RENDER_MODE_WIDE_1080P;
        }
        else if (width >= 1280 && height >= 720)
        {
            fb_width = 1280;
            fb_height = 720;
            tv_render_mode = GX2_TV_RENDER_MODE_WIDE_720P;
        }
        else if (width >= 850 && height >= 480)
        {
            fb_width = 854;
            fb_height = 480;
            tv_render_mode = GX2_TV_RENDER_MODE_WIDE_480P;
        }
        else // if (width >= 640 && height >= 480)
        {
            fb_width = 640;
            fb_height = 480;
            tv_render_mode = GX2_TV_RENDER_MODE_STANDARD_480P;
        }

        // Calculate TV scan buffer byte size
        u32 tv_scan_buffer_size, unk;
        GX2CalcTVSize(
            tv_render_mode,                       // Render Mode
            GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, // Scan Buffer Surface Format
            GX2_BUFFERING_MODE_DOUBLE,            // Two buffers for double-buffering
            &tv_scan_buffer_size,                 // Output byte size
            &unk                                  // Unknown; seems like we have no use for it
        );

        // Allocate TV scan buffer
        gTvScanBuffer = MEMAllocFromDefaultHeapEx(
            tv_scan_buffer_size,
            GX2_SCAN_BUFFER_ALIGNMENT // Required alignment
        );

        if (!gTvScanBuffer)
        {
            WindowExit();
            return false;
        }

        // Flush allocated buffer from CPU cache
        GX2Invalidate(GX2_INVALIDATE_MODE_CPU, gTvScanBuffer, tv_scan_buffer_size);

        // Set the current TV scan buffer
        GX2SetTVBuffer(
            gTvScanBuffer,                        // Scan Buffer
            tv_scan_buffer_size,                  // Scan Buffer Size
            tv_render_mode,                       // Render Mode
            GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, // Scan Buffer Surface Format
            GX2_BUFFERING_MODE_DOUBLE             // Enable double-buffering
        );

        // Set the current TV scan buffer dimensions
        GX2SetTVScale(fb_width, fb_height);
    }

    // Allocate DRC (Gamepad) scan buffer
    {
        drc_width = 854;
        drc_height = 480;

        // Calculate DRC scan buffer byte size
        u32 drc_scan_buffer_size, unk;
        GX2CalcDRCSize(
            GX2_DRC_RENDER_MODE_SINGLE,           // Render Mode
            GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, // Scan Buffer Surface Format
            GX2_BUFFERING_MODE_DOUBLE,            // Two buffers for double-buffering
            &drc_scan_buffer_size,                // Output byte size
            &unk                                  // Unknown; seems like we have no use for it
        );

        // Allocate DRC scan buffer
        gDrcScanBuffer = MEMAllocFromDefaultHeapEx(
            drc_scan_buffer_size,
            GX2_SCAN_BUFFER_ALIGNMENT // Required alignment
        );

        if (!gDrcScanBuffer)
        {
            WindowExit();
            return false;
        }

        // Flush allocated buffer from CPU cache
        GX2Invalidate(GX2_INVALIDATE_MODE_CPU, gDrcScanBuffer, drc_scan_buffer_size);

        // Set the current DRC scan buffer
        GX2SetDRCBuffer(
            gDrcScanBuffer,                       // Scan Buffer
            drc_scan_buffer_size,                 // Scan Buffer Size
            GX2_DRC_RENDER_MODE_SINGLE,           // Render Mode
            GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, // Scan Buffer Surface Format
            GX2_BUFFERING_MODE_DOUBLE             // Enable double-buffering
        );

        // Set the current DRC scan buffer dimensions
        GX2SetDRCScale(drc_width, drc_height);
    }

    // Initialize color buffer
    gColorBuffer.surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
    gColorBuffer.surface.width = fb_width;
    gColorBuffer.surface.height = fb_height;
    gColorBuffer.surface.depth = 1;
    gColorBuffer.surface.mipLevels = 1;
    gColorBuffer.surface.format = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
    gColorBuffer.surface.aa = GX2_AA_MODE1X;
    gColorBuffer.surface.use = GX2_SURFACE_USE_TEXTURE_COLOR_BUFFER_TV;
    gColorBuffer.surface.mipmaps = NULL;
    gColorBuffer.surface.tileMode = GX2_TILE_MODE_DEFAULT;
    gColorBuffer.surface.swizzle  = 0;
    gColorBuffer.viewMip = 0;
    gColorBuffer.viewFirstSlice = 0;
    gColorBuffer.viewNumSlices = 1;
    GX2CalcSurfaceSizeAndAlignment(&gColorBuffer.surface);
    GX2InitColorBufferRegs(&gColorBuffer);

    // Allocate color buffer data
    gColorBufferImageData = MEMAllocFromDefaultHeapEx(
        gColorBuffer.surface.imageSize, // Data byte size
        gColorBuffer.surface.alignment  // Required alignment
    );

    if (!gColorBufferImageData)
    {
        WindowExit();
        return false;
    }

    gColorBuffer.surface.image = gColorBufferImageData;

    // Flush allocated buffer from CPU cache
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU, gColorBufferImageData, gColorBuffer.surface.imageSize);

    // Initialize depth buffer
    gDepthBuffer.surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
    gDepthBuffer.surface.width = fb_width;
    gDepthBuffer.surface.height = fb_height;
    gDepthBuffer.surface.depth = 1;
    gDepthBuffer.surface.mipLevels = 1;
    gDepthBuffer.surface.format = GX2_SURFACE_FORMAT_FLOAT_R32;
    gDepthBuffer.surface.aa = GX2_AA_MODE1X;
    gDepthBuffer.surface.use = GX2_SURFACE_USE_TEXTURE | GX2_SURFACE_USE_DEPTH_BUFFER;
    gColorBuffer.surface.mipmaps = NULL;
    gDepthBuffer.surface.tileMode = GX2_TILE_MODE_DEFAULT;
    gDepthBuffer.surface.swizzle  = 0;
    gDepthBuffer.viewMip = 0;
    gDepthBuffer.viewFirstSlice = 0;
    gDepthBuffer.viewNumSlices = 1;
    gDepthBuffer.hiZPtr = NULL;
    gDepthBuffer.hiZSize = 0;
    gDepthBuffer.depthClear = 1.0f;
    gDepthBuffer.stencilClear = 0;
    GX2CalcSurfaceSizeAndAlignment(&gDepthBuffer.surface);
    GX2InitDepthBufferRegs(&gDepthBuffer);

    // Allocate depth buffer data
    gDepthBufferImageData = MEMAllocFromDefaultHeapEx(
        gDepthBuffer.surface.imageSize, // Data byte size
        gDepthBuffer.surface.alignment  // Required alignment
    );

    if (!gDepthBufferImageData)
    {
        WindowExit();
        return false;
    }

    gDepthBuffer.surface.image = gDepthBufferImageData;

    // Flush allocated buffer from CPU cache
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU, gDepthBufferImageData, gDepthBuffer.surface.imageSize);

    // Allocate context state instance
    gContext = (GX2ContextState*)MEMAllocFromDefaultHeapEx(
        sizeof(GX2ContextState),    // Size of context
        GX2_CONTEXT_STATE_ALIGNMENT // Required alignment
    );

    if (!gContext)
    {
        WindowExit();
        return false;
    }

    // Initialize it to default state and make it current
    GX2SetupContextStateEx(gContext, false);
    WindowMakeContextCurrent();

    // Set the default viewport and scissor
    GX2SetViewport(0, 0, fb_width, fb_height, 0.0f, 1.0f);
    GX2SetScissor(0, 0, fb_width, fb_height);

    if (pWidth)
        *pWidth = fb_width;
    if (pHeight)
        *pHeight = fb_height;

    // TODO: ProcUI

#endif

    WindowSetSwapInterval(1);

    gInitialized = true;
    return true;
}

void WindowMakeContextCurrent()
{
#ifdef TEST_WIN
    glfwMakeContextCurrent(gWindowHandleWin);
#else
    GX2SetContextState(gContext);
    GX2SetColorBuffer(&gColorBuffer, GX2_RENDER_TARGET_0);
    GX2SetDepthBuffer(&gDepthBuffer);
#endif
}

void WindowSetSwapInterval(u32 swap_interval)
{
#ifdef TEST_WIN
    glfwSwapInterval(swap_interval);
#else
    GX2SetSwapInterval(swap_interval);
#endif
}

bool WindowIsRunning()
{
#ifdef TEST_WIN
    return !glfwWindowShouldClose(gWindowHandleWin);
#else
    // TODO: ProcUI
    return true;
#endif
}

void WindowSwapBuffers()
{
#ifdef TEST_WIN

    glfwSwapBuffers(gWindowHandleWin);
    glfwPollEvents();

#else

    // Copy the color buffer to the TV and DRC scan buffers
    GX2CopyColorBufferToScanBuffer(&gColorBuffer, GX2_SCAN_TARGET_TV);
    GX2CopyColorBufferToScanBuffer(&gColorBuffer, GX2_SCAN_TARGET_DRC);
    // Flip
    GX2SwapScanBuffers();

    // Make sure TV and DRC are enabled
    GX2SetTVEnable(true);
    GX2SetDRCEnable(true);

    // Makes sure all GX2 commands queued to the GPU have all been issued and completed
    //GX2DrawDone();

    // No need to call GX2DrawDone() here

    // Since the last commands issued are copying the color buffer to the scan buffers
    // and flipping them, GX2WaitForFlip() will suffice in place of GX2DrawDone()
    // while also applying our frame-rate-limiting policy set by the swap interval

    GX2WaitForFlip();

#endif
}

void WindowExit()
{
#ifdef TEST_WIN
    glfwTerminate();
#else
    // TODO: There is currently no way to exit the application until I implement ProcUI
#endif
}

#ifdef TEST_GX2

GX2ColorBuffer* WindowGetColorBuffer()
{
    return &gColorBuffer;
}

GX2DepthBuffer* WindowGetDepthBuffer()
{
    return &gDepthBuffer;
}

#endif
