
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <cstdio>

#define DEBUG false

GLboolean keysInUse[512];
GLboolean keysCanToggle[512];

bool keyInUse(int key) {
    return keysInUse[key];
}

bool getAndSetKeySinglePress(int key){
    if (key < 0 || key >= 512) return false; // Remove all keys outside allowed interval
    if (keysCanToggle[key]) {
        if (keysInUse[key]) {
            keysCanToggle[key] = false;
            return true;
        }
    }
    return false;
}

void toggleBoolOnPress(bool &b, int key) {
    if (getAndSetKeySinglePress(key)) {
        b = !b;
        if (DEBUG) printf("Toggle key: %i to %i\n", key, b);
    }
}

void handleKeyboardInputs(int key, int action)
{
    // Keep track of pressed/released buttons
    if (key >= 0 && key < 512)
    {
        if (action == GLFW_PRESS)
        {
            if (keysCanToggle[key]) {
                keysInUse[key] = true;
            }
        }
        else if (action == GLFW_RELEASE)
        {
            keysInUse[key] = false;
            keysCanToggle[key] = true;
        }
    }
}