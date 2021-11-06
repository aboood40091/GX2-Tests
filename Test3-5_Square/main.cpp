// Drawing a square
// Based on second half of:
// https://learnopengl.com/Getting-started/Hello-Triangle

#include <window/window.h>

#ifdef TEST_WIN

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#else // TEST_GX2

#include "../Test3_Hello_Triangle/triangle_gx2.hpp"

#include <coreinit/memdefaultheap.h>
#include <gx2/clear.h>
#include <gx2/draw.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/utils.h>

#endif

int main()
{
    u32 fb_width, fb_height;
    if (!WindowInit(1280, 720, &fb_width, &fb_height))
        return -1;

    /*        Make window context current        */

    // No need, automatically done by WindowInit()
    //WindowMakeContextCurrent();

    /*        Quick introduction        */

    // This is an extension to test 3, which covers the second half of the LearnOpenGL example.
    // It shows you how to draw indexed elements; that is, drawing with an index buffer.

    // There are two ways to draw with an index buffer in OpenGL; with a VAO bound:
    // 1. Create an Element Buffer Object (EBO), bind it and bind its data. The offset of the first
    //    element in the EBO buffer data is then passed to the draw function, "glDrawElements".
    // 2. With no EBO currently bound, the pointer to the index buffer is passed to "glDrawElements".

    // As opposed to OpenGL, GX2 doesn't deal with objects; it has no equivalent to the first method.
    // Index buffers are dealt with in the same way as method 2; the index buffer pointer must be
    // passed to the draw function when drawing.

    // The LearnOpenGL example used the first method, but for the sake of comparison with GX2,
    // the second method will be used here instead for the OpenGL version of this example.
    // This is inefficient since the index buffer will be copied to the GPU on every draw call.
    // However, that isn't a problem on Wii U since the index buffer memory is shared between the CPU
    // and GPU and no copying happens between them.

    // Note: GX2 treats index buffers the same as attribute buffers when it comes to shared memory:
    // * Cache must be invalidated and data can be altered or freed only after the draw call is done.
    // * Alignment is not a requirement, but there is a recommended value (32 for index buffers).

    /*        Create Shader Program        */

#ifdef TEST_WIN

    // Vertex Shader Source
    const char* vertex_shader_src =
        "#version 330 core\n"
        "layout(location = 0) in vec3 v_inPos;\n\n"

        "void main()\n"
        "{\n"
        "    gl_Position = vec4(v_inPos, 1.0);\n"
        "}\n";

    // Create vertex shader object
    u32 vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    // Set its source code
    glShaderSource(vertex_shader, 1, &vertex_shader_src, NULL);
    // Compile the source code
    glCompileShader(vertex_shader);

    // Fragment Shader Code
    const char* fragment_shader_src =
        "#version 330 core\n"
        "out vec4 o_FragColor;\n\n"

        "void main()\n"
        "{\n"
        "    o_FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
        "}\n";

    // Create fragment shader object
    u32 fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    // Set its source code
    glShaderSource(fragment_shader, 1, &fragment_shader_src, NULL);
    // Compile the source code
    glCompileShader(fragment_shader);

    // Create shader program object
    u32 shader_program = glCreateProgram();
    // Attach vertex shader to it
    glAttachShader(shader_program, vertex_shader);
    // Attach fragment shader to it
    glAttachShader(shader_program, fragment_shader);
    // Link shaders of the program
    glLinkProgram(shader_program);

    // Delete vertex shader object as it's no longer needed
    glDeleteShader(vertex_shader);
    // Delete fragment shader object as it's no longer needed
    glDeleteShader(fragment_shader);

#else // TEST_GX2

    // The shaders have been compiled externally and stored in triangle_gx2.hpp (included)
    // (to avoid the necessity of adding external file reading to this example)
    // - triangle_VSH: GX2VertexShader instance (the compiled vertex shader)
    // - triangle_PSH: GX2PixelShader instance (the compiled fragment shader)

    // Flush CPU cache and invalidate GPU cache
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, triangle_VSH.program, triangle_VSH.size);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, triangle_PSH.program, triangle_PSH.size);

#endif

    /*        Create VBO        */

    // Positions of the square vertices
    const f32 pos_data[] = {
         0.5f,  0.5f, 0.0f,  // top right
         0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
        -0.5f,  0.5f, 0.0f   // top left
    };
    // Index buffer
    const u32 idx_data[] = {
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };

#ifdef TEST_WIN

    // Generate a single VAO and use it as default
    u32 VAO;
    glGenVertexArrays(1, &VAO);
    // Bind it
    glBindVertexArray(VAO);

    // Generate VBO
    u32 VBO;
    glGenBuffers(1, &VBO);
    // Bind it
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Set data of currently bound VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(pos_data), pos_data, GL_STATIC_DRAW);

    // We will not use an EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_NONE);

#else // TEST_GX2

    // Make sure to flush CPU cache and invalidate GPU cache
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, (void*)pos_data, sizeof(pos_data));

    // Index of vertex buffer slot we are going to use
    u32 VBO = 0;
    // Set its data
    GX2SetAttribBuffer(
        VBO,               // Vertex buffer slot to bind the data to
        sizeof(pos_data),  // Size of buffer data
        3 * sizeof(float), // Stride (Size of each vertex)
        pos_data           // Buffer data
    );

    // Make sure to flush CPU cache and invalidate GPU cache for the index buffer too
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, (void*)idx_data, sizeof(idx_data));

#endif

    /*        Describe our vertex attributes for the vertex fetch stage        */

#ifdef TEST_WIN

    // VBO is already bound at this stage

    // Set vertex attribute at location 0 (position)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

#else // TEST_GX2

    // Create fetch shader
    GX2FetchShader triangle_FSH;

    // In this example, we only have 1 attribute, position, a vec3, meaning only 1 stream is needed
    GX2AttribStream pos_stream;
    pos_stream.location = 0;
    pos_stream.buffer = VBO;
    pos_stream.offset = 0;
    pos_stream.format = GX2_ATTRIB_FORMAT_FLOAT_32_32_32;
    pos_stream.mask = GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_Y, GX2_SQ_SEL_Z, GX2_SQ_SEL_1);
    pos_stream.endianSwap = GX2_ENDIAN_SWAP_DEFAULT;
    pos_stream.type = GX2_ATTRIB_INDEX_PER_VERTEX;
    pos_stream.aluDivisor = 0;

    // Initialize the fetch shader
    // First, allocate memory for the fetch shader program
    u32 triangle_FSH_size = GX2CalcFetchShaderSizeEx(
        1,                                  // Number of attribute streams
        GX2_FETCH_SHADER_TESSELLATION_NONE, // No Tessellation
        GX2_TESSELLATION_MODE_DISCRETE      // ^^^^^^^^^^^^^^^
    );
    void* triangle_FSH_program = MEMAllocFromDefaultHeapEx(
        triangle_FSH_size,
        GX2_SHADER_PROGRAM_ALIGNMENT // Required alignment
    );

    // Actually create program and initialize the fetch shader
    GX2InitFetchShaderEx(
        &triangle_FSH,                      // Fetch shader instance
        (u8*)triangle_FSH_program,          // Allocated shader program memory
        1,                                  // Number of attribute streams
        &pos_stream,                        // Attribute streams
        GX2_FETCH_SHADER_TESSELLATION_NONE, // No Tessellation
        GX2_TESSELLATION_MODE_DISCRETE      // ^^^^^^^^^^^^^^^
    );

    // Make sure to flush CPU cache and invalidate GPU cache
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, triangle_FSH.program, triangle_FSH.size);

#endif

    /*        Set shader program in use before rendering        */

#ifdef TEST_WIN

    // Set the shader program in use
    glUseProgram(shader_program);

#else // TEST_GX2

    // Set shader mode to uniform register (using fixed, common values)
    GX2SetShaderModeEx(GX2_SHADER_MODE_UNIFORM_REGISTER, 48, 64, 0, 0, 200, 192);

    // Set our shaders in use
    GX2SetFetchShader(&triangle_FSH);
    GX2SetVertexShader(&triangle_VSH);
    GX2SetPixelShader(&triangle_PSH);

#endif

    /*        Wireframe mode        */

#ifdef TEST_WIN

    // Set polygon mode
    glPolygonMode(
        GL_FRONT_AND_BACK,
#ifdef TEST_3_5_WIREFRAME
        GL_LINE
#else
        GL_FILL
#endif
    );

#else // TEST_GX2

    // Set polygon mode
    // (Culling and polygon offset enable must also be specified)
    GX2SetPolygonControl(
        GX2_FRONT_FACE_CCW, // Front-face Mode
        FALSE,              // Disable Culling
        FALSE,              // ^^^^^^^^^^^^^^^
        TRUE,               // Enable Polygon Mode
#ifdef TEST_3_5_WIREFRAME
        GX2_POLYGON_MODE_LINE, // Front Polygon Mode
        GX2_POLYGON_MODE_LINE, // Back Polygon Mode
#else
        GX2_POLYGON_MODE_TRIANGLE, // Front Polygon Mode
        GX2_POLYGON_MODE_TRIANGLE, // Back Polygon Mode
#endif
        FALSE, // Disable Polygon Offset
        FALSE, // ^^^^^^^^^^^^^^^^^^^^^^
        FALSE  // ^^^^^^^^^^^^^^^^^^^^^^
    );

#endif

    while (WindowIsRunning())
    {
        /*        Clear the color buffer        */

        // Window context should already be current at this point

#ifdef TEST_WIN

        // Set the current clear color to the given color
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

        // Clear the current color buffer
        glClear(GL_COLOR_BUFFER_BIT);

#else // TEST_GX2

        // Clear the window color buffer explicitly with the given color
        GX2ClearColor(WindowGetColorBuffer(), 0.2f, 0.3f, 0.3f, 1.0f);
        // Restore the window context
        WindowMakeContextCurrent();

#endif

        /*        Draw the triangle        */

#ifdef TEST_WIN
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)idx_data);
#else // TEST_GX2
        GX2DrawIndexedEx(GX2_PRIMITIVE_MODE_TRIANGLES, 6, GX2_INDEX_TYPE_U32, (void*)idx_data, 0, 1);
#endif

        WindowSwapBuffers();
    }

    /*        Free resources        */

#ifdef TEST_WIN

    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
    glDeleteBuffers(1, &VBO);

    glBindVertexArray(GL_NONE);
    glDeleteVertexArrays(1, &VAO);

    glUseProgram(GL_NONE);
    glDeleteProgram(shader_program);

#else // TEST_GX2

    // TODO: The program can't break from the main loop yet

#endif

    WindowExit();
    return 0;
}
