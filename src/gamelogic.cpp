#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <utilities/shader.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <utilities/timeutils.h>
#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fmt/format.h>
#include "gamelogic.h"
#include "objects/sceneGraph.hpp"
#include "utilities/imageLoader.hpp"
#include "objects/ship.h"
#include "objects/shipManager.h"
#include "utilities/camera.hpp"
#include <ThreadPool.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <utilities/buttonHandler.h>
#include <objects/box.h>

Gloom::Camera camera;

SceneNode* rootNode;
SceneNode* boxNode;
SceneNode* sunNode;
SceneNode* asteroidNode;
SceneNode* sunLightNode;
SceneNode* botsTeam;

#define DEFAULT_ALLOWED_BOTS 300
std::vector <Ship*> bots;

std::vector<SceneNode *> SceneNode::collisionObjects;

glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 600.f);

// These are heap allocated, because they should not be initialised at the start of the program
Gloom::Shader* defaultShader;
Gloom::Shader* skyBoxShader;
unsigned int skyBoxTextureID;

const glm::vec3 boxDimensions(250.0f, 250.0f, 250.0f);
const double sunRadius = 15.0f;
const glm::vec3 sunPosition(0, 0, 0);

CommandLineOptions options;

ThreadPool pool(std::max(std::thread::hardware_concurrency(), (unsigned int) 2));
bool useMultiThread = false;

bool captureMouse = true; // A must for debugging as opengl steals the mouse
bool isPaused = true;

void mouseCallback(GLFWwindow* window, double x, double y) {
    if (captureMouse) {
        camera.handleCursorPosInput(x, y);
    }
}

void placeLight3fvVal(int id, const std::string& field, glm::vec3 &v3) {
    std::string uniformName = fmt::format("pointLights[{}].{}", id, field);
    GLint location = defaultShader->getUniformFromName(uniformName);
    glUniform3fv(location, 1, glm::value_ptr(v3));
}

void initGame(GLFWwindow* window, CommandLineOptions gameOptions) {


    options = gameOptions;

    if (captureMouse) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(window, mouseCallback);
    }


    const std::string relativePath = "../"; // Depends on where you build it from,  default clion: ../,  default msvc: ../../../

    defaultShader = new Gloom::Shader();
    defaultShader->makeBasicShader(relativePath + "res/shaders/default.vert", relativePath + "res/shaders/default.frag");
    defaultShader->activate();

    skyBoxShader = new Gloom::Shader();
    skyBoxShader->makeBasicShader(relativePath + "res/shaders/skybox.vert", relativePath +"res/shaders/skybox.frag");

    // Configuration of skybox
    PNGImage skyBoxTexture = loadPNGFile(relativePath + "res/textures/space1.png");
    skyBoxTextureID = getTextureID(&skyBoxTexture);

    // Create meshes
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh sphere = generateSphere(1.0f, 15, 15);


    // Construct scene
    rootNode = new SceneNode(SceneNode::GROUP);

    // Init and configure sun node
    unsigned int sphereVAO = generateBuffer(sphere);
    sunNode = new SceneNode();
    rootNode->addChild(sunNode);
    sunNode->vertexArrayObjectID = (int) sphereVAO;
    sunNode->VAOIndexCount = sphere.indices.size();
    sunNode->material.baseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    sunNode->position = sunPosition;
    sunNode->scale = glm::vec3(sunRadius);
    sunNode->rotation = {0, 0, 0 };
    sunNode->ignoreLight = 1;
    sunNode->boundingBoxDimension = glm::vec3(1.0f * 2.0f + 0.1f); // Sphere radius, not sunScaleRadius + a bit extra
    sunNode->hasBoundingBox = true;
    sunNode->hasTinyBoundingBox = true;
    sunNode->tinyBoundingBoxSize = (float)sunRadius; // Approximate AABB with radius
    SceneNode::collisionObjects.push_back(sunNode);


    asteroidNode = new SceneNode();
    sunNode->addChild(asteroidNode);
    asteroidNode->vertexArrayObjectID = (int) sphereVAO;
    asteroidNode->VAOIndexCount = sphere.indices.size();
    asteroidNode->material.baseColor = glm::vec3(0.641f);
    asteroidNode->position = glm::vec3(-30.0f, 0.0f, 50.0f) * 1.0f/sunNode->scale;
    asteroidNode->scale = glm::vec3(4.0f) * 1.0f/sunNode->scale; // Counteract the scaling (due to sphere mechanism)
    asteroidNode->rotation = {0, 0, 0 };
    asteroidNode->boundingBoxDimension = glm::vec3(1.0f * 2.0f + 1.0f);
    asteroidNode->hasBoundingBox = true;
    asteroidNode->hasTinyBoundingBox = true;
    asteroidNode->tinyBoundingBoxSize = 4.0f; // Approximate AABB with radius
    SceneNode::collisionObjects.push_back(asteroidNode);
    Ship::attractors.push_back(asteroidNode);

    // Lights
    sunLightNode = new SceneNode();

    sunLightNode->nodeType = SceneNode::POINT_LIGHT;
    sunLightNode->lightSourceID = 0;
    sunLightNode->position = glm::vec3(0.0f, 0.0, 0.0f);
    sunLightNode->setStaticMat();

    glm::vec3 c;
    c = glm::vec3(0.7f, 0.7f, 0.7f);
    placeLight3fvVal(sunLightNode->lightSourceID, "ambientColor", c);
    placeLight3fvVal(sunLightNode->lightSourceID, "diffuseColor", c);
    placeLight3fvVal(sunLightNode->lightSourceID, "specularColor", c);
    sunNode->addChild(sunLightNode);

    // Configuration of box node
    boxNode = new Box(boxDimensions, true);
    rootNode->addChild(boxNode);
    boxNode->position = glm::vec3(0.0f);
    boxNode->hasBoundingBox = true;
    boxNode->boundingBoxDimension = boxDimensions;
    boxNode->setStaticMat(); // Speed up matrix calculation as the object is not moved
    SceneNode::collisionObjects.push_back(boxNode);


    // Textures, normal and roughness maps can be loaded like this
    //PNGImage brickTextureMap = loadPNGFile("../res/textures/Brick03_col.png");
    //PNGImage brickNormalMap = loadPNGFile("../res/textures/Brick03_nrm.png");
    //PNGImage brickRoughMap = loadPNGFile("../res/textures/Brick03_rgh.png");
    //unsigned int brickTextureID = getTextureID(&brickTextureMap);
    //unsigned int brickNormalTextureID = getTextureID(&brickNormalMap);
    //unsigned int brickRoughMapID = getTextureID(&brickRoughMap);
    //boxNode->textureID = brickTextureID;
    //boxNode->normalMapTextureID = brickNormalTextureID;
    //boxNode->roughnessMapID = brickRoughMapID;

    // Init and configure bots node
    botsTeam = new SceneNode(SceneNode::GROUP);
    botsTeam->setStaticMat();
    rootNode->addChild(botsTeam);

    for (int i=0; i<DEFAULT_ALLOWED_BOTS; i++) {
        Ship* ship = new Ship();
        bots.push_back(ship);
        botsTeam->addChild(ship); // Add it to be rendered
    }
    bots.at(0)->generateLaser(); // Lazy fix for race condition (Note implement better cache structure) or pre-init
    bots.at(0)->lasers.at(0)->enabled = false;

    //GLfloat lineWidthRange[2];
    //glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
    glLineWidth(1);

    double upstartTime = getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with (ish) {} SceneNodes in {} seconds.", rootNode->totalChildren(), upstartTime) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;

    // Debug settings
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glDisable(GL_CULL_FACE);
}

std::vector<int> mouseKeys = {GLFW_MOUSE_BUTTON_1, GLFW_MOUSE_BUTTON_2};
std::vector<int> keys = {GLFW_KEY_K, GLFW_KEY_M, GLFW_KEY_B,
                         GLFW_KEY_F1, GLFW_KEY_F3, GLFW_KEY_F4,
                         GLFW_KEY_F7, GLFW_KEY_F8};
void handleKeyboardInputGameLogic(GLFWwindow* window) {
    // Update all keys
    for (int key : keys) {
        handleKeyboardInputs(key, glfwGetKey(window, key));
    }
    for (int key : mouseKeys) {
        handleKeyboardInputs(key, glfwGetMouseButton(window, key));
    }

    // Toggle pausing
    toggleBoolOnPress(isPaused, GLFW_MOUSE_BUTTON_2);

    // Spawn lasers on all ships
    if (getAndSetKeySinglePress(GLFW_MOUSE_BUTTON_1)) {
        for (Ship *s : bots) {
            s->generateLaser();
        }
    }

    /* Debugg keybindings */

    // Toggle use of multithread
    toggleBoolOnPress(useMultiThread, GLFW_KEY_M);

    // Toggle mouse lock (usefull for debugging)
    if (getAndSetKeySinglePress(GLFW_KEY_K)) {
        captureMouse = !captureMouse;
        if (captureMouse) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    // Disable drawing of box
    if (getAndSetKeySinglePress(GLFW_KEY_B)) {
        boxNode->enabled = !boxNode->enabled;
        Ship::disableSafetyNet = !boxNode->enabled;
    }

    if (keyInUse(GLFW_KEY_F8)) {
        Ship* ship = new Ship();
        bots.push_back(ship);
        botsTeam->addChild(ship);
    }

    if (keyInUse(GLFW_KEY_F7)) {
        int index = bots.size()-1;
        assert(bots.at(index) == botsTeam->children.at(index));
        Ship * ship = bots.at(index);
        bots.erase(bots.begin() + index);
        botsTeam->children.erase(botsTeam->children.begin() + index);
        delete ship;
    }

    if (getAndSetKeySinglePress(GLFW_KEY_F1)) {
        printf("Help:\n"
               "  Movement:      WASD\n"
               "  Up/down:       E/Q\n"
               "  Pause:         Mouse click 2\n"
               "  Force shoot:   Mouse click 1\n"
               "  Gimbal lock:   G\n"
               "  Multi thread:  M\n"
               "  Toggle box:    B\n"
               "  Help:          F1\n"
               "  Camera pos:    F3\n"
               "  Show status:   F4\n"
               "  Disable mouse: K\n"
               "  Add/Sub ships: F8/F7\n"
               );
    }

    if (getAndSetKeySinglePress(GLFW_KEY_F3)) {
        printf("Camera\n"
               "  possition: (%f, %f, %f)\n",
               camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);
    }

    if (getAndSetKeySinglePress(GLFW_KEY_F4)) {
        printf("Status: \n"
               "  Pause:       %i\n"
               "  Multithread: %i\n"
               "  Mouselock:   %i\n"
               "  Box status:  %i\n"
               "  Bots:        %i\n",
               isPaused, useMultiThread, captureMouse, boxNode->enabled, (int)bots.size());
    }
}

int frameCount=0;
double sumTimeDelta=0;
void updateFrame(GLFWwindow* window) {

    double timeDelta = getTimeDeltaSeconds();

    // FPS estimate
    // Also responsible for adaptive bots amount
    frameCount++;
    sumTimeDelta += timeDelta;
    if (sumTimeDelta > 2.0f) {
        float fps = ((float)frameCount/(float)sumTimeDelta);
        if (!isPaused) updateAmountBots(bots, botsTeam, fps, sumTimeDelta);
        printf("FPS: %f\n", fps);
        frameCount = 0;
        sumTimeDelta = 0;
    }

    // Handle keyboard input
    handleKeyboardInputGameLogic(window);

    // Gamelogic
    if (!isPaused) {

        // Sun rotating
        sunNode->rotation.y = std::fmod(sunNode->rotation.y - timeDelta/3.0f, 360.0f);
        asteroidNode->rotation.y += timeDelta/2.0f;

        // Update all bots
        //bots.at(0).printShip();
        std::vector<std::future<void>> futures;
        auto updS = [](Ship* &ship, double &timeDelta, std::vector<Ship *> &bots) {return ship->updateShip(timeDelta, bots);};
        for (unsigned int i=0; i < bots.size(); i++) {
            if (useMultiThread) {
                // enqueue and store future
                futures.push_back(pool.enqueue(updS, bots.at(i), timeDelta, bots));
            } else {
                bots.at(i)->updateShip(timeDelta, bots);
            }
        }

        for (auto &future : futures) {
            future.get();
        }
    }

    // Move camera and calculate view matrix
    camera.detectKeyboardInputs(window);
    camera.updateCamera(timeDelta);
    glm::mat4 cameraTransform = camera.getViewMatrix();
    glm::mat4 VP = projection * cameraTransform;

    // update uniforms that doesnt change that often (once per draw)
    glUniform3fv(10, 1, glm::value_ptr(camera.getCameraPosition()));

    glm::vec4 asteroidNodePos = asteroidNode->currentModelTransformationMatrix*glm::vec4(0.0f,0.0f,0.0f,1.0f);
    glUniform3fv(11, 1, glm::value_ptr(asteroidNodePos));

    updateNodeTransformations(rootNode, VP, glm::mat4(1.0f));
}

void updateNodeTransformations(SceneNode* node, glm::mat4 VP, glm::mat4 transformationThusFar) {
    glm::mat4 transformationMatrix;
    if (!node->staticRefScaleRot){
        transformationMatrix =
                glm::translate(node->position)
                * glm::translate(node->referencePoint)
                * glm::rotate(node->rotation.y, glm::vec3(0,1,0))
                * glm::rotate(node->rotation.x, glm::vec3(1,0,0))
                * glm::rotate(node->rotation.z, glm::vec3(0,0,1))
                * glm::scale(node->scale)
                * glm::translate(-node->referencePoint);
    } else {
        transformationMatrix = glm::translate(node->position)
                             * node->refScaleRot;
    }

    // Projection*View * Model
    node->currentModelTransformationMatrix = transformationThusFar * transformationMatrix; // M
    node->currentTransformationMatrix = VP * node->currentModelTransformationMatrix; // MVP

    node->worldPos = glm::vec3(node->currentModelTransformationMatrix*glm::vec4(0.0f,0.0f,0.0f,1.0f));


    // Compute transpose of the inverse of the model matrix
    // To fix normals inside the shader
    // https://stackoverflow.com/questions/10879864/what-extractly-mat3a-mat4-matrix-statement-in-glsl-do
    node->currentNormalMatrix = glm::mat3(glm::transpose(glm::inverse(node->currentModelTransformationMatrix)));

    switch(node->nodeType) {
        case SceneNode::POINT_LIGHT:

            break;
        case SceneNode::GEOMETRY: break;
        case SceneNode::GEOMETRY_NORMAL_MAPPED: break;
        case SceneNode::GROUP: break;
        case SceneNode::LINE: break;
    }

    std::vector<std::future<void>> futures;
    for(SceneNode* child : node->children) {
        assert(child != node);
        if (useMultiThread) {
            futures.push_back(pool.enqueue(updateNodeTransformations, child, VP, node->currentModelTransformationMatrix));
        } else {
            updateNodeTransformations(child, VP, node->currentModelTransformationMatrix);
        }
    }

    if (node->getIndependentChildrenSize() > 0) {
        for (SceneNode* child : node->getIndependentChildren()) {
            if (useMultiThread && false) { //disabled as it overloads the thread-framework OR creates a weird race condition TODO
                futures.push_back(pool.enqueue(updateNodeTransformations, child, VP, transformationThusFar));
            } else {
                updateNodeTransformations(child, VP, transformationThusFar);
            }
        }
    }

    // Wait for all threads to finish
    for (std::future<void> &a : futures) {
        a.get();
    }
}

void renderNode(SceneNode* node) {
    if (node->enabled) {
        // MVP
        glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
        // Matrix M
        glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(node->currentModelTransformationMatrix));
        // pass normal matrix to the vertex shader
        glUniformMatrix3fv(5, 1, GL_FALSE, glm::value_ptr(node->currentNormalMatrix));

        // Set object material
        if (node->vertexArrayObjectID != -1) {
            GLint location = defaultShader->getUniformFromName("material.baseColor");
            glUniform3fv(location, 1, glm::value_ptr(node->material.baseColor));
            location = defaultShader->getUniformFromName("material.shininess");
            glUniform1f(location, node->material.shininess);
        }

        glUniform1i(12, node->ignoreLight); // Enable / disable lightning calculations

        switch(node->nodeType) {
            case SceneNode::GEOMETRY:
                if(node->vertexArrayObjectID != -1) {
                    glBindVertexArray((GLuint)(node->vertexArrayObjectID));
                    glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
                }
                break;
            case SceneNode::LINE:
                {
                    if(node->vertexArrayObjectID != -1) {
                        glBindVertexArray((GLuint)node->vertexArrayObjectID);
                        glDrawElements(GL_LINES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
                    }
                }
                break;
            case SceneNode::GEOMETRY_NORMAL_MAPPED:
                {
                    glUniform1i(7, 1); // useTexture
                    glUniform1i(8, 1); // useNormalMap
                    glUniform1i(9, 1); // useRoughnessMap
                    glBindTextureUnit(1, node->textureID);
                    glBindTextureUnit(2, node->normalMapTextureID);
                    glBindTextureUnit(3, node->roughnessMapID);

                    if(node->vertexArrayObjectID != -1) {
                        glBindVertexArray((GLuint)node->vertexArrayObjectID);
                        glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
                    }
                    glUniform1i(7, 0);
                    glUniform1i(8, 0);
                    glUniform1i(9, 0);
                }
                break;
            case SceneNode::POINT_LIGHT:
                {
                    glm::vec4 pos = node->currentModelTransformationMatrix*glm::vec4(0.0f,0.0f,0.0f,1.0f);
                    glm::vec3 pos3 = glm::vec3(pos)/pos.w;  // Correct the length
                    std::string uniformName = fmt::format("pointLights[{}].position", node->lightSourceID);
                    GLint location = defaultShader->getUniformFromName(uniformName);
                    glUniform3fv(location, 1, glm::value_ptr(pos3));
                }
                break;
            case SceneNode::GROUP: break;
        }
    }

    for(SceneNode* child : node->children) {
        renderNode(child);
    }

    if (node->getIndependentChildrenSize() > 0) {
        for (SceneNode* child : node->getIndependentChildren()) {
            renderNode(child);
        }
    }
}

void renderSkybox(){
    skyBoxShader->activate();
    glDepthMask(GL_FALSE);
    glBindTextureUnit(0, skyBoxTextureID);

    glm::mat4 cameraTransform = camera.getViewMatrixRotOnly();
    glm::mat4 VP = projection * cameraTransform;
    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(VP));

    glBindVertexArray((GLuint)(boxNode->vertexArrayObjectID));
    glDrawElements(GL_TRIANGLES, boxNode->VAOIndexCount, GL_UNSIGNED_INT, nullptr);

    glDepthMask(GL_TRUE);
    defaultShader->activate(); // Return to default shader

}

void renderFrame(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, (GLint)(windowWidth), (GLint)(windowHeight));
    renderSkybox();
    renderNode(rootNode);
}
