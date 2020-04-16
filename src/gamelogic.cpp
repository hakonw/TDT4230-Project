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
#include "objects/laser.h"
#include <ThreadPool.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <utilities/buttonHandler.h>

enum KeyFrameAction {
    BOTTOM, TOP
};

Gloom::Camera camera;

SceneNode* rootNode;
SceneNode* boxNode;
SceneNode* boxUnitNode;
SceneNode* sunNode;

SceneNode* sunLightNode;
SceneNode* staticLightNode;
SceneNode* padLightNode;

glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);

#define DEFAULT_ALLOWED_BOTS 300
std::vector <Ship*> bots;
//std::vector <Ship> &Ship::ships = bots;

SceneNode* botsTeamA;

void renderNode(SceneNode* node);

#define NUM_POINT_LIGHTS 3

double sunRadius = 15.0f;

// These are heap allocated, because they should not be initialised at the start of the program
Gloom::Shader* defaultShader;
Gloom::Shader* skyBoxShader;
Gloom::Shader* blurShader;
unsigned int skyBoxTextureID;

unsigned int hdrFBO;
unsigned int colorBuffers[2];
unsigned int pingpongFBO[2];
unsigned int pingpongColorbuffers[2];

//const glm::vec3 boxDimensions(180, 90, 90);
//const glm::vec3 boxDimensions(180*1.5f, 90*1.5f, 90*1.5f);
const glm::vec3 boxDimensions(250.0f, 250.0f, 250.0f);

glm::vec3 sunPosition(0, 0, 0);

CommandLineOptions options;

ThreadPool pool(std::max(std::thread::hardware_concurrency(), (unsigned int) 2));
bool useMultiThread = false;

bool captureMouse = true; // A must for debugging as opengl steals the mouse

bool isPaused = true;

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

    if (captureMouse) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        glfwSetCursorPosCallback(window, mouseCallback);
    }


    defaultShader = new Gloom::Shader();
    defaultShader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    defaultShader->activate();

    skyBoxShader = new Gloom::Shader();
    skyBoxShader->makeBasicShader("../res/shaders/skybox.vert", "../res/shaders/skybox.frag");

    blurShader = new Gloom::Shader();
    blurShader->makeBasicShader("../res/shaders/blur.vert", "../res/shaders/blur.frag");
    printf("Blur shader status: %i\n|", blurShader->isValid());

    // Configuration of skybox
    PNGImage skyBoxTexture = loadPNGFile("../res/textures/space1.png");
    skyBoxTextureID = getTextureID(&skyBoxTexture);

    // Create meshes
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh sphere = generateSphere(1.0f, 10, 10);
    Mesh unitBox = cube(glm::vec3(2.0f), glm::vec2(1.0f), false, true);

    unsigned int boxUnitVAO = generateBuffer(unitBox);
    boxUnitNode = new SceneNode();
    boxUnitNode->vertexArrayObjectID = boxUnitVAO;
    boxUnitNode->VAOIndexCount = box.indices.size();
    boxUnitNode->nodeType = SceneNode::GEOMETRY;

    // Construct scene
    rootNode = new SceneNode();

    // Init and configure sun node
    unsigned int sphereVAO = generateBuffer(sphere);
    sunNode = new SceneNode();
    rootNode->addChild(sunNode);
    sunNode->vertexArrayObjectID = sphereVAO;
    sunNode->VAOIndexCount = sphere.indices.size();
    sunNode->material.baseColor = glm::vec3(1.0f, 1.0f, 1.0f); // Yellow
    sunNode->position = sunPosition;
    sunNode->scale = glm::vec3(sunRadius);
    sunNode->rotation = {0, 0, 0 };
    sunNode->ignoreLight = 1;
    sunNode->boundingBoxDimension = glm::vec3(1.0f);
    sunNode->hasBoundingBox = true;

    // Init and configure bots node
    botsTeamA = new SceneNode(SceneNode::GROUP);
    rootNode->addChild(botsTeamA);

    for (int i=0; i<DEFAULT_ALLOWED_BOTS; i++) {
        Ship* ship = new Ship();
        bots.push_back(ship);
        botsTeamA->addChild(ship); // Add it to be rendered
        ship->collisionObjects.push_back(sunNode);
    }
    bots.at(0)->generateLaser(); // Lazy for for race condition (Note implement better cache structre) or pre-init

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

    sunNode->addChild(sunLightNode);

    // Configuration of box node
    unsigned int boxVAO  = generateBuffer(box);
    boxNode  = new SceneNode();
    rootNode->addChild(boxNode);
    boxNode->vertexArrayObjectID = boxVAO;
    boxNode->VAOIndexCount = box.indices.size();
    boxNode->nodeType = SceneNode::GEOMETRY;
    boxNode->enabled = false;

    PNGImage brickTextureMap = loadPNGFile("../res/textures/Brick03_col.png");
    //PNGImage brickNormalMap = loadPNGFile("../res/textures/Brick03_nrm.png");
    //PNGImage brickRoughMap = loadPNGFile("../res/textures/Brick03_rgh.png");
    unsigned int brickTextureID = getTextureID(&brickTextureMap);
    //unsigned int brickNormalTextureID = getTextureID(&brickNormalMap);
    //unsigned int brickRoughMapID = getTextureID(&brickRoughMap);

    boxNode->textureID = brickTextureID;
    //boxNode->normalMapTextureID = brickNormalTextureID;
    //boxNode->roughnessMapID = brickRoughMapID;

    //GLfloat lineWidthRange[2];
    //glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
    glLineWidth(1);

    double upstartTime = getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with (ish) {} SceneNodes in {} seconds.", rootNode->totalChildren(), upstartTime) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;

    // set up floating point framebuffer to render scene to
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img->pixels.data());
        glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGB16F, windowWidth, windowHeight, 0, GL_RGB, GL_FLOAT, NULL
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(
                GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0
        );
    }

    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, windowWidth, windowHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, windowWidth, windowHeight, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }

    // Debug settings
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glDisable(GL_CULL_FACE);
}

int frameCount=0;
double sumTimeDelta=0;
std::vector<int> mouseKeys = {GLFW_MOUSE_BUTTON_1, GLFW_MOUSE_BUTTON_2};
std::vector<int> keys = {};
void updateFrame(GLFWwindow* window) {

    if (captureMouse) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    double timeDelta = getTimeDeltaSeconds();

    // FPS estimate
    // Also responsible for adaptive bots amount
    frameCount++;
    sumTimeDelta += timeDelta;
    if (sumTimeDelta > 2.0f) {
        float fps = ((float)frameCount/(float)sumTimeDelta);
        if (!isPaused) updateAmountBots(bots, botsTeamA, fps, sumTimeDelta);
        printf("FPS: %f\n", fps);
        frameCount = 0;
        sumTimeDelta = 0;
    }

    for (int key : keys) {
        handleKeyboardInputs(key, glfwGetKey(window, key));
    }
    for (int key : mouseKeys) {
        handleKeyboardInputs(key, glfwGetMouseButton(window, key));
    }

    // Game logic, pausing
    if (getAndSetKeySinglePress(GLFW_MOUSE_BUTTON_2)) {
        isPaused = not isPaused;
    }
    if (!isPaused) {
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

        if (getAndSetKeySinglePress(GLFW_MOUSE_BUTTON_1)) {
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

    // 1b, save the model projection
    // Projection*View * Model
    node->currentModelTransformationMatrix = transformationThusFar * transformationMatrix; // M
    node->currentTransformationMatrix = VP * node->currentModelTransformationMatrix; // MVP

    // 1d
    // Compute transpose of the inverse of the model matrix
    // Take the mat3 out of it
    // https://stackoverflow.com/questions/10879864/what-extractly-mat3a-mat4-matrix-statement-in-glsl-do
    node->currentNormalMatrix = glm::mat3(glm::transpose(glm::inverse(node->currentModelTransformationMatrix)));

    // push the sun node position to a uniform variable
    // TODO remove if un-needed
    if (node == sunNode) {
        glm::vec4 pos = node->currentModelTransformationMatrix*glm::vec4(0.0f,0.0f,0.0f,1.0f);
        glUniform3fv(11, 1, glm::value_ptr(pos));
    }

    switch(node->nodeType) {
        case SceneNode::POINT_LIGHT:
            {
                glm::vec4 pos = node->currentModelTransformationMatrix*glm::vec4(0.0f,0.0f,0.0f,1.0f);
                glm::vec3 pos3 = glm::vec3(pos)/pos.w;  // Correct the length
                std::string uniformName = fmt::format("pointLights[{}].position", node->lightSourceID);
                GLint location = defaultShader->getUniformFromName(uniformName);
                glUniform3fv(location, 1, glm::value_ptr(pos3));
            }
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
            if (useMultiThread && false) { //temporarly disabled as it creates a weird race condition
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
    glBindTextureUnit(0, skyBoxTextureID);

    glm::mat4 cameraTransform = camera.getViewMatrixRotOnly();
    glm::mat4 VP = projection * cameraTransform;
    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(VP));

    glBindVertexArray((GLuint)(boxNode->vertexArrayObjectID));
    glDrawElements(GL_TRIANGLES, boxNode->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
}


void renderQ() {
    glBindVertexArray((GLuint)(boxUnitNode->vertexArrayObjectID));
    glDrawElements(GL_TRIANGLES, boxUnitNode->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
}

void renderFrame(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, (GLint)(windowWidth), (GLint)(windowHeight));

    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO); // Set multi-buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear passed buffer to avoid windows xp looking bug

    // Draw skybox
    glDepthMask(GL_FALSE);
    skyBoxShader->activate();
    renderSkybox();
    glDepthMask(GL_TRUE);

    // Draw all other things
    defaultShader->activate();
    renderNode(rootNode);

    // Reset to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

/*    // 2. blur bright fragments with two-pass Gaussian Blur
    // --------------------------------------------------
    bool horizontal = true, first_iteration = true;
    unsigned int amount = 1;
    blurShader->activate();
    for (unsigned int i = 0; i < amount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
        glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
        renderSkybox();
        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);*/

    // Draw combined frame
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
    blurShader->activate();
    renderQ();


    defaultShader->activate();
}
