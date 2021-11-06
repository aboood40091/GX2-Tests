// Drawing a triangle
// Based on:
// https://learnopengl.com/Getting-started/Hello-Triangle

#include <window/window.h>

#ifdef TEST_WIN

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

#else // TEST_GX2

#include "triangle_gx2.hpp"

#include <coreinit/memdefaultheap.h>
#include <gx2/clear.h>
#include <gx2/draw.h>
#include <gx2/mem.h>
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

    // Print it to check I didn't mess it up
    //std::cout << vertex_shader_src << std::endl;

    // Create vertex shader object
    u32 vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    // Set its source code
    glShaderSource(vertex_shader, 1, &vertex_shader_src, NULL);
    // Compile the source code
    glCompileShader(vertex_shader);

    // Error-checking...
    {
        int  success;
        char infoLog[512];
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
    }

    // Fragment Shader Code
    const char* fragment_shader_src =
        "#version 330 core\n"
        "out vec4 o_FragColor;\n\n"

        "void main()\n"
        "{\n"
        "    o_FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
        "}\n";

    // Print it to check I didn't mess it up
    //std::cout << fragment_shader_src << std::endl;

    // Create fragment shader object
    u32 fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    // Set its source code
    glShaderSource(fragment_shader, 1, &fragment_shader_src, NULL);
    // Compile the source code
    glCompileShader(fragment_shader);

    // Error-checking...
    {
        int  success;
        char infoLog[512];
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
    }

    // Create shader program object
    u32 shader_program = glCreateProgram();
    // Attach vertex shader to it
    glAttachShader(shader_program, vertex_shader);
    // Attach fragment shader to it
    glAttachShader(shader_program, fragment_shader);
    // Link shaders of the program
    glLinkProgram(shader_program);

    // Error-checking...
    {
        int  success;
        char infoLog[512];
        glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
        if(!success) {
            glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }
    }

    // Delete vertex shader object as it's no longer needed
    glDeleteShader(vertex_shader);
    // Delete fragment shader object as it's no longer needed
    glDeleteShader(fragment_shader);

#else // TEST_GX2

    // OpenGL requires shaders to be compiled at run-time
    // However, GX2 requires the opposite, which is offline shader compilation
    // Sadly, GX2 does not provide any utility for compiling shaders at run-time

    // The shaders have been compiled externally and stored in triangle_gx2.hpp (included)
    // (to avoid the necessity of adding external file reading to this example)
    // - triangle_VSH: GX2VertexShader instance (the compiled vertex shader)
    // - triangle_PSH: GX2PixelShader instance (the compiled fragment shader)

    // A note on caching:

    // Some data, such as shader program data, texture data and uniform block data, is read by the GPU
    // directly from main memory (which is why it must be correctly aligned)
    // In that case, main memory is shared between the CPU and the GPU

    // When that data is created/manipulated by the CPU, the changes apply in the CPU cache before
    // they are applied in the main memory since cache memory is faster to access by the CPU
    // There are cases where the changes may not have been applied to the main memory yet
    // Likewise, there are cases where there already exists data for the shared region in the GPU
    // cache memory, which the GPU will prioritize over the new data in the main memory

    // Therefore, to avoid cache coherency problems, you must do the following for the shared region:
    // * Invalidate the GPU cache (so that the GPU is forced to read from main memory the next time)
    // * Flush the CPU cache to main memory (so that our data manipulation is actually applied in
    //                                       the main memory and the GPU does not read incorrect data)
    // Use GX2Invalidate() to do that
    // (Take a look at the GX2InvalidateMode enum for types of data to call GX2Invalidate() on)

    // Buffers that require special alignment:
    // 1. *are* the buffers that are in shared memory between CPU and GPU
    // 2. *are* the buffers that require cache invalidation

    // In this specific example, flushing the CPU cache for the vertex and fragment shader programs
    // is not needed since I made the data as static variables, but it's good practice
    // (GPU cache must still be invalidated though)
    // GX2_INVALIDATE_MODE_CPU_SHADER = GX2_INVALIDATE_MODE_CPU | GX2_INVALIDATE_MODE_SHADER
    // * GX2_INVALIDATE_MODE_CPU: Flush CPU cache to main memory
    // * GX2_INVALIDATE_MODE_SHADER: Invalidate shader program cache on the GPU
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, triangle_VSH.program, triangle_VSH.size);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, triangle_PSH.program, triangle_PSH.size);

    // In OpenGL, you must create a shader program object, attach the vertex
    // and fragment shaders and link them, then you can use the shader program whenever needed
    // and the vertex and fragment shader objects can be disposed of

    // In GX2, the idea of a shader program object does not exist; you must directly set
    // the GX2 shader instances in use wherever you would have originally set
    // the shader program in use in OpenGL

#endif

    /*        Create VBO        */

    // Positions of the triangle vertices
    const f32 pos_data[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
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

#else // TEST_GX2

    // GX2 does not have the concept of objects
    // However, it has 16 vertex attribute buffers slots (0-15)
    // OpenGL requires you to generate and bind a VBO,
    // but we will be dealing with fixed VB slots in GX2

    // Unlike OpenGL which allows for different strides
    // per attribute in a VBO, GX2 requires all attributes
    // to have the same stride per VB slot

    // You will only need to use several VB slots when the
    // stride cannot be made to match for all attributes

    // All VB slots can be used per vertex shader program
    // if the user wishes to

    // For our example, we only need one slot since all
    // attributes (just position, for now) have the same stride

    // Note: attribute buffers (pos_data, in this case) are the only type of buffers that
    // do *not* require special alignment, but it is recommended that they are aligned by 64
    // (They do still require cache invalidation)

    // Make sure to flush CPU cache and invalidate GPU cache
    // (GX2_INVALIDATE_MODE_ATTRIBUTE_BUFFER: Invalidate attribute buffer cache on the GPU)
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

    // WARNING: Bare in mind that no GX2 function makes a copy of your buffers, unlike OpenGL
    // Do *not* free pos_data or overwrite it until it's no longer in use by the GPU
    // (i.e. until the drawcall is done)
    // The same applies to all types of buffers that require cache invalidation
    // (To force the program to wait until the drawcall is done, call GX2DrawDone() after it)

#endif

    /*        Describe our vertex attributes for the vertex fetch stage        */

#ifdef TEST_WIN

    // VBO is already bound at this stage

    // Set vertex attribute at location 0 (position)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

#else // TEST_GX2

    // In GX2, you must explicitly create a fetch shader for the vertex fetch stage
    GX2FetchShader triangle_FSH;

    // Then, you need to feed it with attribute streams

    // For each attribute you are going to pass to the vertex shader,
    // you need to create an attribute stream

    // However, keep in mind that a single attribute stream can only hold elements the size of vec4
    // Therefore, all basic types (int, float, bool, vec2, vec3, vec4) can be stored
    // in a single attribute stream (all elements smaller than a vec4 are padded to the size of vec4)

    // (double-precision fp types are not supported by GX2)

    // The same cannot be said for arrays of any of these basic types (int[], vec3[], etc.)
    // As their total size is a *multiple* of vec4
    // (You can think of it as: GX2 requires the GLSL std140 memory layout, even for attributes)
    // Likewise for matrices (i.e. mat32, mat43, mat4, etc.)

    // In that case, you are required to create an attribute stream for each element in the array
    // (location increments sequentially for each element in the array)

    // Hopefully we will cover that in a future example
    // In this example, we only have 1 attribute, position, a vec3, meaning only 1 stream is needed

    GX2AttribStream pos_stream;

    // Initialize the stream

    // The location of the attribute (we set it in the shader source, 0 in this case)
    // Note: the location can be determined at runtime from the GX2VertexShader instance
    // (Must be incremented for each array element, not the case here though)
    pos_stream.location = 0;

    // The buffer slot currently holding the data of this attribute
    pos_stream.buffer = VBO;

    // Offset to the first instance of this attribute in the buffer (0 in this case)
    // (Should be incremented for each array element, not the case here though)
    pos_stream.offset = 0;

    // Format of the data (vec3 in this case)
    pos_stream.format = GX2_ATTRIB_FORMAT_FLOAT_32_32_32;

    // Vector component swizzling; how the attribute will be padded to vec4 if it's smaller
    // Non-vector types, such as int, can be assumed to be a vector of size 1
    // In this case, I would like the resulting vec4 to be (in.x, in.y, in.z, 1)
    pos_stream.mask = GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_Y, GX2_SQ_SEL_Z, GX2_SQ_SEL_1);

    // Perform endian swap when reading the attribute data from the buffer
    // Keep in mind that the Wii U CPU is big-endian, whereas the GPU is little-endian
    // In this case, my data is in big endian, therefore, it needs to be swapped when reading
    // (Default will do correct swapping based on the format we set)
    pos_stream.endianSwap = GX2_ENDIAN_SWAP_DEFAULT;

    // Whether the attribute advances per vertex or per instance
    pos_stream.type = GX2_ATTRIB_INDEX_PER_VERTEX;
    // Divisor
    // Enabled only if type is per-vertex
    // Same as what you would set with glVertexAttribDivisor
    // Other than the value 1, you can have up to two unique divisors per fetch shader
    // Value of 0 will force the type back to per-vertex for this attribute
    pos_stream.aluDivisor = 0;

    // The attribute stream is now properly initialized
    // Time to initialize the fetch shader

    // First, allocate memory for the fetch shader program
    // (The user is responsible for freeing it when the shader is no longer needed)
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

    // In GX2, you must set the shader mode

    // It must be done *before* you set any shaders in use when setting
    // the shader mode for the first time or changing it

    // Set shader mode to uniform register (using fixed, common values)
    // Hopefully will be explained in a future example where uniforms are relevant
    GX2SetShaderModeEx(GX2_SHADER_MODE_UNIFORM_REGISTER, 48, 64, 0, 0, 200, 192);

    // Set our shaders in use
    GX2SetFetchShader(&triangle_FSH);
    GX2SetVertexShader(&triangle_VSH);
    GX2SetPixelShader(&triangle_PSH);

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

        // GX2 does not provide any function to clear the current color buffer,
        // nor a function to get the current color buffer

        // Clear the window color buffer explicitly with the given color
        // Does not need a current context to be set
        GX2ClearColor(WindowGetColorBuffer(), 0.2f, 0.3f, 0.3f, 1.0f);

        // GX2ClearColor invalidates the current context and the window context
        // must be made current again
        WindowMakeContextCurrent();

#endif

        /*        Draw the triangle        */

#ifdef TEST_WIN
        glDrawArrays(GL_TRIANGLES, 0, 3);
#else // TEST_GX2
        GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLES, 3, 0, 1);
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
