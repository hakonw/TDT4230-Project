// Local headers
#include "program.hpp"
#include "utilities/window.hpp"
#include "gamelogic.h"

void runProgram(GLFWwindow* window, CommandLineOptions options)
{
    // Enable depth (Z) buffer (accept "closest" fragment)
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Configure miscellaneous OpenGL settings
    glEnable(GL_CULL_FACE);

    // Disable built-in dithering
    glDisable(GL_DITHER);

    // Enable transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set default colour after clearing the colour buffer
    glClearColor(0.3f, 0.5f, 0.8f, 1.0f);

	initGame(window, options);

    // Rendering Loop
    while (!glfwWindowShouldClose(window))
    {
	    // Clear colour and depth buffers
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        updateFrame(window);
        renderFrame(window);
        printGLError();

        // Handle other events
        glfwPollEvents();
        handleKeyboardInput(window);

        // Flip buffers
        glfwSwapBuffers(window);
    }
}


void handleKeyboardInput(GLFWwindow* window)
{
    // Use escape key for terminating the GLFW window
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}
