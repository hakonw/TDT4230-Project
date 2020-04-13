
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <assert.h>

GLboolean keysInUse[512];
GLboolean keysCanToggle[512];

bool keyInUse(int key) {
    assert(0<=key);
    assert(key<512);
    return keysInUse[key];
}

bool getAndSetKeySinglePress(int key){
    if (keysCanToggle[key]) {
        if (keysInUse[key]) {
            keysCanToggle[key] = false;
            return true;
        }
    }
    return false;
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