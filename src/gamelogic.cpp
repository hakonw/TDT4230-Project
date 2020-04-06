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
#include "objects/ShipManager.h"
#include "utilities/camera.hpp"
#include "objects/laser.h"
#include <ThreadPool.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

enum KeyFrameAction {
    BOTTOM, TOP
};

Gloom::Camera camera;

double padPositionX = 0;
double padPositionZ = 0;

SceneNode* rootNode;
SceneNode* boxNode;
SceneNode* sunNode;

SceneNode* sunLightNode;
SceneNode* staticLightNode;
SceneNode* padLightNode;

glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);

#define DEFAULT_ALLOWED_BOTS 100
std::vector <Ship*> bots;
//std::vector <Ship> &Ship::ships = bots;

SceneNode* botsTeamA;


// I am mostly lazy
void updateAmountBots(float &currentFps, double &time);
unsigned int getTextureID(PNGImage* img);
void renderNode(SceneNode* node);

#define NUM_POINT_LIGHTS 3

double sunRadius = 15.0f;

// These are heap allocated, because they should not be initialised at the start of the program
Gloom::Shader* defaultShader;
Gloom::Shader* skyBoxShader;
unsigned int skyBoxTextureID;

//const glm::vec3 boxDimensions(180, 90, 90);
//const glm::vec3 boxDimensions(180*1.5f, 90*1.5f, 90*1.5f);
const glm::vec3 boxDimensions(250.0f, 250.0f, 250.0f);

glm::vec3 sunPosition(0, 0, 0);

CommandLineOptions options;

ThreadPool pool(std::max(std::thread::hardware_concurrency(), (unsigned int) 2));
bool useMultiThread = false;

bool isPaused = true;

bool mouseLeftPressed   = false;
bool mouseLeftCanToggle = true;
bool mouseLeftReleased  = false;
bool mouseRightPressed  = false;
bool mouseRightReleased = false;

void mouseCallback(GLFWwindow* window, double x, double y) {
    camera.handleCursorPosInput(x, y);
}

void placeLight3fvVal(int id, std::string field, glm::vec3 v3) {
    std::string uniformName = fmt::format("pointLights[{}].{}", id, field);
    GLint location = defaultShader->getUniformFromName(uniformName);
    glUniform3fv(location, 1, glm::value_ptr(v3));
}

void initGame(GLFWwindow* window, CommandLineOptions gameOptions) {
    options = gameOptions;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseCallback);

    defaultShader = new Gloom::Shader();
    defaultShader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    defaultShader->activate();

    skyBoxShader = new Gloom::Shader();
    skyBoxShader->makeBasicShader("../res/shaders/skybox.vert", "../res/shaders/skybox.frag");


    // Create meshes
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh sphere = generateSphere(1.0f, 10, 10);

    // Fill buffers
    unsigned int sphereVAO = generateBuffer(sphere);
    unsigned int boxVAO  = generateBuffer(box);

    // Construct scene
    rootNode = new SceneNode();
    boxNode  = new SceneNode();
    sunNode = new SceneNode();

    rootNode->addChild(boxNode);
    rootNode->addChild(sunNode);

    botsTeamA = new SceneNode(SceneNode::GROUP);
    rootNode->addChild(botsTeamA);

    for (int i=0; i<DEFAULT_ALLOWED_BOTS; i++) {
        Ship* ship = new Ship();
        bots.push_back(ship);
        botsTeamA->addChild(ship); // Add it to be rendered
    }

    // Lights
    sunLightNode = new SceneNode();
    padLightNode = new SceneNode();
    staticLightNode = new SceneNode();

    sunLightNode->nodeType = SceneNode::POINT_LIGHT;
    sunLightNode->lightSourceID = 0;
    sunLightNode->position = glm::vec3(0.0f, 0.0, 0.0f);

    padLightNode->nodeType = SceneNode::POINT_LIGHT;
    padLightNode->lightSourceID = 1;
    padLightNode->position = glm::vec3(0.0f, 0.0f, -1.0f);

    staticLightNode->nodeType = SceneNode::POINT_LIGHT;
    staticLightNode->lightSourceID = 2;
    staticLightNode->position = glm::vec3(5.0f, 0.0f, -1.0f);

    // From task 1, where each light had to be at a different place
    //bots.at(0)->addChild(sunLightNode);
    sunNode->addChild(sunLightNode);

    glm::vec3 c;
    c = glm::vec3(0.2f, 0.0f, 0.0f);
    c = glm::vec3(0.7f, 0.7f, 0.7f);
    placeLight3fvVal(sunLightNode->lightSourceID, "ambientColor", c);
    placeLight3fvVal(sunLightNode->lightSourceID, "diffuseColor", c);
    placeLight3fvVal(sunLightNode->lightSourceID, "specularColor", c);

    c = glm::vec3(0.0f, 0.0f, 0.2f);
    placeLight3fvVal(padLightNode->lightSourceID, "ambientColor", c);
    placeLight3fvVal(padLightNode->lightSourceID, "diffuseColor", c);
    placeLight3fvVal(padLightNode->lightSourceID, "specularColor", c);

    c = glm::vec3(0.0f, 0.2f, 0.0f);
    placeLight3fvVal(staticLightNode->lightSourceID, "ambientColor", c);
    placeLight3fvVal(staticLightNode->lightSourceID, "diffuseColor", c);
    placeLight3fvVal(staticLightNode->lightSourceID, "specularColor", c);

    // Back to skeleton

    boxNode->vertexArrayObjectID = boxVAO;
    boxNode->VAOIndexCount = box.indices.size();
    boxNode->nodeType = SceneNode::GEOMETRY_NORMAL_MAPPED;

    sunNode->vertexArrayObjectID = sphereVAO;
    sunNode->VAOIndexCount = sphere.indices.size();
    sunNode->material.baseColor = glm::vec3(1.0f, 1.0f, 0.0f); // Yellow
    sunNode->position = sunPosition;
    sunNode->scale = glm::vec3(sunRadius);
    sunNode->rotation = {0, 0, 0 };
    sunNode->ignoreLight = 1;


    // Task 3 b
    PNGImage brickTextureMap = loadPNGFile("../res/textures/Brick03_col.png");
    PNGImage brickNormalMap = loadPNGFile("../res/textures/Brick03_nrm.png");
    PNGImage brickRoughMap = loadPNGFile("../res/textures/Brick03_rgh.png");
    unsigned int brickTextureID = getTextureID(&brickTextureMap);
    unsigned int brickNormalTextureID = getTextureID(&brickNormalMap);
    unsigned int brickRoughMapID = getTextureID(&brickRoughMap);

    boxNode->normalMapTextureID = brickNormalTextureID;
    boxNode->textureID = brickTextureID;
    boxNode->roughnessMapID = brickRoughMapID;

    PNGImage skyBoxTexture = loadPNGFile("../res/textures/space1.png");
    skyBoxTextureID = getTextureID(&skyBoxTexture);

    //GLfloat lineWidthRange[2];
    //glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
    glLineWidth(1);

    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", rootNode->totalChildren()) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;

    // Debug settings
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glDisable(GL_CULL_FACE);
}

unsigned int getTextureID(PNGImage* img) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img->pixels.data());

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}


// Adaptive update bots based on fps
void updateAmountBots(float &currentFps, double &time) {
    // Only update bots if inside limits
    if (bots.size() > minBots && bots.size() < maxBots) {
        int deltaBots = calculateAmountAdaptiveUpdateAmountBots(currentFps, time);
        if (bots.size() + deltaBots < minBots) deltaBots = bots.size() - minBots;

        if (deltaBots > 0) { // Add bots
            // Activate old disabled bots
            for (Ship *s : bots) {
                if (deltaBots == 0) break;
                if (!s->enabled) {
                    s->enabled = true;
                    deltaBots--;
                }
            }
            // Add new bots if needed
            for (int i=0; i<deltaBots; i++) {
                Ship *ship = new Ship();
                bots.push_back(ship);
                botsTeamA->addChild(ship); // Add it to be rendered
            }
            printf("adaptive bot amount: Increasing bots with %i to a total of %lu\n", deltaBots, bots.size());

        } else if (deltaBots < 0) { // Remove bots
            // Clean up old disabled bots, and then remove bots
            unsigned int disabled = 0;
            unsigned int deleted = 0;
            int tmpDeltaBots = deltaBots;
            for (unsigned int i=bots.size()-1; i>minBots; i--) {
                // TODO fix possible O(n^2), but has a low call rate, low pri
                if (!bots.at(i)->enabled) {
                    deleted++;
                    Ship* s = bots.at(i);
                    assert(bots.at(i) == botsTeamA->children.at(i));
                    bots.erase(bots.begin()+i);
                    botsTeamA->children.erase(botsTeamA->children.begin()+i);
                    delete s;
                } else {
                    if (tmpDeltaBots < 0) {
                        bots.at(i)->enabled = false;
                        tmpDeltaBots++; // Work towards 0
                        disabled++;
                    } else if (tmpDeltaBots == 0) {
                        break;
                    }
                }
            }
            printf("adaptive bot amount: Decreasing bots with %i (%i disabled, %i deleted) to a size of %lu active and a total %lu\n", deltaBots, disabled, deleted, bots.size()-disabled, bots.size());
        }
    }
}

int frameCount=0;
int frameCount2=0;
double sumTimeDelta=0;
double sumTimeDelta2=0;
void updateFrame(GLFWwindow* window) {

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double timeDelta = getTimeDeltaSeconds();
    sumTimeDelta2 += timeDelta;
    if (frameCount2++ % 100 == 0) {
        printf("TimeDelta average last 100 frames: %f\n", sumTimeDelta2/100);
        sumTimeDelta2 = 0.0f;
    }

    // FPS estimate
    // Also responsible for adaptive bots amount
    frameCount++;
    sumTimeDelta += timeDelta;
    if (sumTimeDelta > 2.0f) {
        float fps = ((float)frameCount/(float)sumTimeDelta);
        if (!isPaused) updateAmountBots(fps, sumTimeDelta);
        printf("FPS: %f\n", fps);
        frameCount = 0;
        sumTimeDelta = 0;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) {
        mouseLeftPressed = true;
        mouseLeftReleased = false;
    } else {
        mouseLeftReleased = mouseLeftPressed;
        mouseLeftPressed = false;
        mouseLeftCanToggle = true;
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2)) {
        mouseRightPressed = true;
        mouseRightReleased = false;
    } else {
        mouseRightReleased = mouseRightPressed;
        mouseRightPressed = false;
    }

    // Game logic, pausing
    if (isPaused) {
        if (mouseRightReleased) {
            isPaused = false;
        }
    } else {
        if (mouseRightReleased) {
            isPaused = true;
        }

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

        if (mouseLeftPressed && mouseLeftCanToggle) {
            mouseLeftCanToggle = false;
            mouseLeftPressed = false;
            for (Ship* s : bots) {
                s->generateLaser();
            }
        }
    }


    // Move camera and calculate view matrix
    camera.detectKeyboardInputs(window);
    camera.updateCamera(timeDelta);
    glm::mat4 cameraTransform = camera.getViewMatrix();

    glm::mat4 VP = projection * cameraTransform;

    // Move and rotate various SceneNodes
    boxNode->position = { 0, -10, 0 };


    // update uniforms that doesnt change that often (once per draw)
    glUniform3fv(10, 1, glm::value_ptr(camera.getCameraPosition()));
    updateNodeTransformations(rootNode, VP, glm::mat4(1.0f));
}

void updateNodeTransformations(SceneNode* node, glm::mat4 VP, glm::mat4 transformationThusFar) {
    glm::mat4 transformationMatrix =
              glm::translate(node->position)
            * glm::translate(node->referencePoint)
            * glm::rotate(node->rotation.y, glm::vec3(0,1,0))
            * glm::rotate(node->rotation.x, glm::vec3(1,0,0))
            * glm::rotate(node->rotation.z, glm::vec3(0,0,1))
            * glm::scale(node->scale)
            * glm::translate(-node->referencePoint);

    // 1b, save the model projection
    // Projection*View * Model
    node->currentModelTransformationMatrix = transformationThusFar * transformationMatrix; // M
    node->currentTransformationMatrix = VP * node->currentModelTransformationMatrix; // MVP

    // 1d
    // Compute transpose of the inverse of the model matrix
    // Take the mat3 out of it
    // https://stackoverflow.com/questions/10879864/what-extractly-mat3a-mat4-matrix-statement-in-glsl-do
    node->currentNormalMatrix = glm::mat3(glm::transpose(glm::inverse(node->currentModelTransformationMatrix)));

    // push the ball node position to a uniform variable
    if (node == sunNode) {
        glm::vec4 pos = node->currentModelTransformationMatrix*glm::vec4(0.0f,0.0f,0.0f,1.0f);
        glUniform3fv(11, 1, glm::value_ptr(pos));
    }

    switch(node->nodeType) {
        case SceneNode::GEOMETRY: break;
        case SceneNode::GEOMETRY_NORMAL_MAPPED: break;
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
            if (useMultiThread) {
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
            // TODO use uniform buffer instead?
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
            case SceneNode::POINT_LIGHT: break;
            case SceneNode::GROUP: break;
            case SceneNode::LINE:
                {
                    if(node->vertexArrayObjectID != -1) {
                        glBindVertexArray((GLuint)node->vertexArrayObjectID);
                        glDrawElements(GL_LINES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
                    }
                }
                break;
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
