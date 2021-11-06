# GX2-Tests
Provided are GX2 test programs and samples for comparison with OpenGL and with additional GLFW+OpenGL implementation for test on PC.  
These samples will be part of an introductory tutorial to GX2 for people with background in modern OpenGL (v3.3 and later).  

## Introduction
(TODO: Move to the wiki)  
  
GX2:  
* The graphics API on the Wii U.  
* Heavily inspired by OpenGL v3.3.  

### Key-differences
* Unlike OpenGL, you do not use a separate API for window handling. GX2 is to be used to set up the screen and the like.  
* Unlike OpenGL, GX2 does not remember any parameters you pass to it. They are queued to the GPU. Therefore, you cannot use it to get specific parameters from the current state (unless there exists a fixed function for it, such as screen size). The only way to save and restore the current state is through using a GX2ContextState instance, which, when set as current, is automatically updated when the current state is modified.  
    In most cases, that does not make a big difference in usage. GX2 has functions that change the current state and others that rely on the current state, just like OpenGL.  
* Unlike OpenGL, GX2 is just a thin wrapper over the GPU. It does not do any management for you, such as copying data from the CPU to the GPU. Buffers allocated by the user are sent directly to the GPU. Therefore, the user must be cautious about GPU constraints when allocating and using data, such as required alignment and caching. (More on this later)  
* Similarly to OpenGL, GX2 is written in C, a language that does not involve OOP (Object-Oriented-Programming). However, unlike OpenGL, GX2 does not have the concept of objects either. As previously mentioned, GX2 queues your commands to the GPU and is just a thin wrapper. It does not save any data you pass to it. Anything you set will be applied to the current GPU state/context.  

## What's in here?
* Test 1: Simple Hello World program.  
* Test 2: Simple test program for creating the "window" (GLFW window on PC, TV screen on Wii U). Renders animated colors through the color buffer clear color.  
* Test 3: Port of Hello Triangle example from LearnOpenGL.  
    Test 3.5: Second half of the Hello Triangle example from LearnOpenGL. Draws a square (with optional wireframe mode).  
