#ifndef CAMERA_HPP
#define CAMERA_HPP
#pragma once

// System headers
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <GLFW/glfw3.h>


namespace Gloom
{
    class Camera
    {
    public:
        Camera(glm::vec3 position         = glm::vec3(0.0f, 0.0f, 0.0f),
               GLfloat   movementSpeed    = 30.0f,
               GLfloat   mouseSensitivity = 0.001f,
               GLfloat   movementSpeedModifierNormal = 1.0f,
               GLfloat   movementSpeedModifierActive = 3.0f)
        {
            cPosition         = position;
            cMovementSpeed    = movementSpeed;
            cMouseSensitivity = mouseSensitivity;
            cMovementSpeedModifier = movementSpeedModifierNormal;
            cMovementSpeedModifierNormal = movementSpeedModifierNormal;
            cMovementSpeedModifierActive = movementSpeedModifierActive;
            assert(cMovementSpeedModifierNormal != cMovementSpeedModifierActive);

            // Set up the initial view matrix
            updateViewMatrix();
        }

        // Public member functions

        /* Getter for the view matrix */
        glm::mat4 getViewMatrix() { return matView; }

        glm::mat4 getViewMatrixRotOnly() { return matViewRot; }

        /* Handle keyboard button presses */
        std::vector<int> keys = {GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, // WASD - Movement
                                 GLFW_KEY_Q, GLFW_KEY_E, // Q E - Up/down
                                 GLFW_KEY_LEFT_SHIFT, GLFW_KEY_G}; // "Sprint" and Gimbal-Lock
        void detectKeyboardInputs(GLFWwindow* window) {
            for (int key : keys) {
                handleKeyboardInputs(key, glfwGetKey(window, key));
            }
        }

        /* Handle keyboard inputs from a callback mechanism */
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


        /* Handle mouse button inputs from a callback mechanism */
        void handleMouseButtonInputs(int button, int action)
        {
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
            {
                isMousePressed = true;
            }
            else
            {
                isMousePressed = false;
                resetMouse = true;
            }
        }


        /* Handle cursor position from a callback mechanism */
        void handleCursorPosInput(double xpos, double ypos)
        {
            // There should be no movement when the mouse button is released
            if (resetMouse)
            {
                lastXPos = xpos;
                lastYPos = ypos;
                resetMouse = false;
            }

            // Keep track of pitch and yaw for the current frame
            fYaw   += xpos - lastXPos;
            fPitch += ypos - lastYPos;
            //printf("Yaw %f \n", fYaw);

            // Update last known cursor position
            lastXPos = xpos;
            lastYPos = ypos;
        }


        /* Update the camera position and view matrix
           `deltaTime` is the time between the current and last frame */
        void updateCamera(GLfloat deltaTime)
        {
            // Extract movement information from the view matrix
            glm::vec3 dirX(matView[0][0], matView[1][0], matView[2][0]);
            glm::vec3 dirY(matView[0][1], matView[1][1], matView[2][1]);
            glm::vec3 dirZ(matView[0][2], matView[1][2], matView[2][2]);

            // Alter position in the appropriate direction
            glm::vec3 fMovement(0.0f, 0.0f, 0.0f);

            if (keysInUse[GLFW_KEY_LEFT_SHIFT]) { // Boost
                if (cMovementSpeedModifier != cMovementSpeedModifierActive) {
                    cMovementSpeedModifier = cMovementSpeedModifierActive;
                }
            } else {
                if (cMovementSpeedModifier != cMovementSpeedModifierNormal) {
                    cMovementSpeedModifier = cMovementSpeedModifierNormal;
                }
            }

            if (keysInUse[GLFW_KEY_G]) { // Gimbal lock - toggle
                // TODO https://en.wikipedia.org/wiki/Rotation_formalisms_in_three_dimensions#Conversion_formulae_between_formalisms ?
                keysInUse[GLFW_KEY_G] = false;
                keysCanToggle[GLFW_KEY_G] = false;
                gimbalLock = !gimbalLock;
                printf("Toggle %i\n", gimbalLock);
            }

            if (keysInUse[GLFW_KEY_W])  // forward
                fMovement -= dirZ;

            if (keysInUse[GLFW_KEY_S])  // backward
                fMovement += dirZ;

            if (keysInUse[GLFW_KEY_A])  // left
                fMovement -= dirX;

            if (keysInUse[GLFW_KEY_D])  // right
                fMovement += dirX;

            if (keysInUse[GLFW_KEY_E])  // vertical up
                fMovement += dirY;

            if (keysInUse[GLFW_KEY_Q])  // vertical down
                fMovement -= dirY;

            // Trick to balance PC speed with movement
            GLfloat velocity = cMovementSpeed * cMovementSpeedModifier * deltaTime;

            // Update camera position using the appropriate velocity
            cPosition += fMovement * velocity;

            // Update the view matrix based on the new information
            updateViewMatrix();
        }

        const glm::vec3 &getCameraPosition() const {
            return cPosition;
        }

    private:
        // Disable copying and assignment
        Camera(Camera const &) = delete;
        Camera & operator =(Camera const &) = delete;

        // Private member function

        /* Update the view matrix based on the current information */
        void updateViewMatrix()
        {
            glm::mat4 matRotation;

            if (gimbalLock) {
                // Adjust cursor movement using the specified sensitivity, but not overwrite current
                GLfloat fPitch2 = fPitch  * cMouseSensitivity;
                GLfloat fYaw2   = fYaw * cMouseSensitivity;

                // Create quaternions given the current pitch and yaw
                glm::quat qPitch = glm::quat(glm::vec3(fPitch2, 0.0f, 0.0f));
                glm::quat qYaw   = glm::quat(glm::vec3(0.0f, fYaw2, 0.0f));

                qPitch = glm::normalize(qPitch);
                qYaw = glm::normalize(qYaw);

                // Do not use old quaterion, create a new mat4 based on only yaw and pitch
                glm::mat4 yawMat = glm::mat4_cast(qYaw);
                glm::mat4 pitchMat = glm::mat4_cast(qPitch);

                matRotation = pitchMat * yawMat;
            } else {
                // Adjust cursor movement using the specified sensitivity
                fPitch *= cMouseSensitivity;
                fYaw   *= cMouseSensitivity;

                // Create quaternions given the current pitch and yaw
                glm::quat qPitch = glm::quat(glm::vec3(fPitch, 0.0f, 0.0f));
                glm::quat qYaw   = glm::quat(glm::vec3(0.0f, fYaw, 0.0f));

                // Reset pitch and yaw values for the current rotation
                fPitch = 0.0f;
                fYaw   = 0.0f;

                // Update camera quaternion and normalise
                cQuaternion = qYaw * qPitch * cQuaternion;
                cQuaternion = glm::normalize(cQuaternion);

                // Build rotation matrix using the camera quaternion
                matRotation = glm::mat4_cast(cQuaternion);
            }

            // Build translation matrix
            glm::mat4 matTranslate = glm::translate(glm::mat4(1.0f), -cPosition);

            // Update view matrix
            matViewRot = matRotation;
            matView = matRotation * matTranslate;
        }

        // Private member variables

        // Camera quaternion and frame pitch and yaw
        glm::quat cQuaternion;
        GLfloat fPitch = 0.0f;
        GLfloat fYaw   = 0.0f;
        GLboolean gimbalLock = false;

        // Camera position
        glm::vec3 cPosition;

        // Variables used for bookkeeping
        GLboolean resetMouse     = true;
        GLboolean isMousePressed = false;
        GLboolean keysInUse[512];
        GLboolean keysCanToggle[512];

        // Last cursor position
        GLfloat lastXPos = 0.0f;
        GLfloat lastYPos = 0.0f;

        // Camera settings
        GLfloat cMovementSpeed;
        GLfloat cMouseSensitivity;
        GLfloat cMovementSpeedModifier;
        GLfloat cMovementSpeedModifierNormal;
        GLfloat cMovementSpeedModifierActive;

        // View matrix
        glm::mat4 matView;

        glm::mat4 matViewRot;
    };
}

#endif
