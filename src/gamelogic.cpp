#include <chrono>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <SFML/Audio/SoundBuffer.hpp>
#include <utilities/shader.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <utilities/timeutils.h>
#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include <SFML/Audio/Sound.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fmt/format.h>
#include "gamelogic.h"
#include "sceneGraph.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "utilities/imageLoader.hpp"
#include "ship.h"
#include "ShipManager.h"

enum KeyFrameAction {
    BOTTOM, TOP
};

#include <timestamps.h>

double padPositionX = 0;
double padPositionZ = 0;

unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;

SceneNode* rootNode;
SceneNode* boxNode;
SceneNode* ballNode;
SceneNode* padNode;
SceneNode* ballLightNode;
SceneNode* staticLightNode;
SceneNode* padLightNode;

#define DEFAULT_ALLOWED_BOTS 100
std::vector <Ship> bots;
//std::vector <Ship> &Ship::ships = bots;

SceneNode* botsTeamA;


// I am mostly lazy
void updateAmountBots(float &currentFps, double &time);
unsigned int getTextureID(PNGImage* img);
void renderNode(SceneNode* node);

#define NUM_POINT_LIGHTS 3

double ballRadius = 3.0f;

// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer;
Gloom::Shader* shader;
sf::Sound* sound;

const glm::vec3 boxDimensions(180, 90, 90);
const glm::vec3 padDimensions(30, 3, 40);

glm::vec3 ballPosition(0, ballRadius + padDimensions.y, boxDimensions.z / 2);
glm::vec3 ballDirection(1, 1, 0.2f);

CommandLineOptions options;

bool hasStarted = false;
bool hasLost = false;
bool jumpedToNextFrame = false;
bool isPaused = false;

bool mouseLeftPressed   = false;
bool mouseLeftReleased  = false;
bool mouseRightPressed  = false;
bool mouseRightReleased = false;

// Modify if you want the music to start further on in the track. Measured in seconds.
const float debug_startTime = 0;
double totalElapsedTime = debug_startTime;
double gameElapsedTime = debug_startTime;

double mouseSensitivity = 1.0;
double lastMouseX = windowWidth / 2;
double lastMouseY = windowHeight / 2;
void mouseCallback(GLFWwindow* window, double x, double y) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    double deltaX = x - lastMouseX;
    double deltaY = y - lastMouseY;

    padPositionX -= mouseSensitivity * deltaX / windowWidth;
    padPositionZ -= mouseSensitivity * deltaY / windowHeight;

    if (padPositionX > 1) padPositionX = 1;
    if (padPositionX < 0) padPositionX = 0;
    if (padPositionZ > 1) padPositionZ = 1;
    if (padPositionZ < 0) padPositionZ = 0;

    glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);
}


void placeLight3fvVal(int id, std::string field, glm::vec3 v3) {
    std::string uniformname = fmt::format("pointLights[{}].{}", id, field);
    GLint location = shader->getUniformFromName(uniformname);
    glUniform3fv(location, 1, glm::value_ptr(v3));
}

//// A few lines to help you if you've never used c++ structs
// struct LightSource {
//     bool a_placeholder_value;
// };
// LightSource lightSources[/*Put number of light sources you want here*/];

void initGame(GLFWwindow* window, CommandLineOptions gameOptions) {
    buffer = new sf::SoundBuffer();
    if (!buffer->loadFromFile("../res/Hall of the Mountain King.ogg")) {
        return;
    }

    options = gameOptions;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseCallback);

    shader = new Gloom::Shader();
    shader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader->activate();

    // Create meshes
    Mesh pad = cube(padDimensions, glm::vec2(30, 40), true);
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh sphere = generateSphere(1.0, 40, 40);

    // Fill buffers
    unsigned int ballVAO = generateBuffer(sphere);
    unsigned int boxVAO  = generateBuffer(box);
    unsigned int padVAO  = generateBuffer(pad);

    // Construct scene
    rootNode = createSceneNode();
    boxNode  = createSceneNode();
    padNode  = createSceneNode();
    ballNode = createSceneNode();

    rootNode->children.push_back(boxNode);
    rootNode->children.push_back(padNode);
    rootNode->children.push_back(ballNode);


    botsTeamA = createSceneNode();
    botsTeamA->nodeType = GROUP;
    rootNode->children.push_back(botsTeamA);


    for (int i=0; i<DEFAULT_ALLOWED_BOTS; i++) {
        Ship ship;
        //ship.sceneNode = createSceneNode();
        //ship.generateShipNode();
        bots.push_back(ship);
        botsTeamA->children.push_back(ship.sceneNode); // Add it to be rendered
    }

    // Lights
    // task 1a
    ballLightNode = createSceneNode();
    padLightNode = createSceneNode();
    staticLightNode = createSceneNode();

    ballLightNode->nodeType = SceneNodeType::POINT_LIGHT;
    ballLightNode->lightSourceID = 0;
    ballLightNode->position = glm::vec3(0.0f, ballRadius*1.01f, 0.0f);
    //ballLightNode->position = glm::vec3(-5.0f, 0.0f, -1.0f);

    padLightNode->nodeType = SceneNodeType::POINT_LIGHT;
    padLightNode->lightSourceID = 1;
    padLightNode->position = glm::vec3(0.0f, 0.0f, -1.0f);

    staticLightNode->nodeType = SceneNodeType::POINT_LIGHT;
    staticLightNode->lightSourceID = 2;
    staticLightNode->position = glm::vec3(5.0f, 0.0f, -1.0f);

    // From task 1, where each light had to be at a different place
    ballNode->children.push_back(ballLightNode);
    padNode->children.push_back(ballLightNode);
    padNode->children.push_back(padLightNode);
    //rootNode->children.push_back(staticLightNode);
    padNode->children.push_back(staticLightNode);

    //3b Select node colors
    glm::vec3 c;
    c = glm::vec3(0.2f, 0.0f, 0.0f);
    c = glm::vec3(0.7f, 0.7f, 0.7f);
    placeLight3fvVal(ballLightNode->lightSourceID, "ambientColor", c);
    placeLight3fvVal(ballLightNode->lightSourceID, "diffuseColor", c);
    placeLight3fvVal(ballLightNode->lightSourceID, "specularColor", c);

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
    boxNode->nodeType = GEOMETRY_NORMAL_MAPPED;

    padNode->vertexArrayObjectID = padVAO;
    padNode->VAOIndexCount = pad.indices.size();

    ballNode->vertexArrayObjectID = ballVAO;
    ballNode->VAOIndexCount = sphere.indices.size();


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


    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;
}

// Assigment 2 task 1c, 1d
unsigned int getTextureID(PNGImage* img) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img->pixels.data());

    // Assugment 2 task 1e
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
        if (bots.size() - deltaBots < minBots) deltaBots = bots.size() - minBots;

        if (deltaBots > 0) { // Add bots
            printf("adaptive bot amount: Increasing bots with %i to a total of %i\n", deltaBots, bots.size()+deltaBots);


            // Activate old disabled bots
            for (Ship s : bots) {
                if (deltaBots == 0) break;
                if (!s.enabled) {
                    s.enabled = true;
                    deltaBots--;
                }
            }
            // Add new bots if needed
            for (int i=0; i<deltaBots; i++) {
                Ship ship;
                bots.push_back(ship);
                botsTeamA->children.push_back(ship.sceneNode); // Add it to be rendered
            }

        } else if (deltaBots < 0) { // Remove bots
            printf("adaptive bot amount: Decreasing bots with %i to a total of %i\n", deltaBots, bots.size()+deltaBots);
            // Clean up old disabled bots, and then remove bots
            for (unsigned int i=bots.size()-1; i>minBots; i--) {
                if (!bots.at(i).enabled) {
                    bots.erase(bots.begin()+i);
                } else {
                    if (deltaBots < 0) {
                        bots.at(i).enabled = false;
                        deltaBots++; // Work towards 0
                    }
                }
            }
        }
    }
}

int frameCount=0;
double sumTimeDelta=0;
void updateFrame(GLFWwindow* window) {

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double timeDelta = getTimeDeltaSeconds();

    // FPS estimate
    frameCount++;
    sumTimeDelta += timeDelta;
    if (sumTimeDelta > 2.0f) {
        float fps = ((double)frameCount/sumTimeDelta);
        updateAmountBots(fps, sumTimeDelta);
        printf("FPS: %f\n", fps);
        frameCount = 0;
        sumTimeDelta = 0;

    }
    //printNode(ballNode);

    //bots.at(0).printShip();
    for (unsigned int i=0; i < bots.size(); i++) {
        bots.at(i).updateShip(timeDelta, bots);
    }

    const float ballBottomY = boxNode->position.y - (boxDimensions.y/2) + ballRadius + padDimensions.y;
    const float ballTopY    = boxNode->position.y + (boxDimensions.y/2) - ballRadius;
    const float BallVerticalTravelDistance = ballTopY - ballBottomY;

    const float cameraWallOffset = 30; // Arbitrary addition to prevent ball from going too much into camera

    const float ballMinX = boxNode->position.x - (boxDimensions.x/2) + ballRadius;
    const float ballMaxX = boxNode->position.x + (boxDimensions.x/2) - ballRadius;
    const float ballMinZ = boxNode->position.z - (boxDimensions.z/2) + ballRadius;
    const float ballMaxZ = boxNode->position.z + (boxDimensions.z/2) - ballRadius - cameraWallOffset;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) {
        mouseLeftPressed = true;
        mouseLeftReleased = false;
    } else {
        mouseLeftReleased = mouseLeftPressed;
        mouseLeftPressed = false;
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2)) {
        mouseRightPressed = true;
        mouseRightReleased = false;
    } else {
        mouseRightReleased = mouseRightPressed;
        mouseRightPressed = false;
    }
    
    if(!hasStarted) {
        if (mouseLeftPressed) {
            if (options.enableMusic) {
                sound = new sf::Sound();
                sound->setBuffer(*buffer);
                sf::Time startTime = sf::seconds(debug_startTime);
                sound->setPlayingOffset(startTime);
                sound->play();
            }
            totalElapsedTime = debug_startTime;
            gameElapsedTime = debug_startTime;
            hasStarted = true;
        }

        ballPosition.x = ballMinX + (1 - padPositionX) * (ballMaxX - ballMinX);
        ballPosition.y = ballBottomY;
        ballPosition.z = ballMinZ + (1 - padPositionZ) * ((ballMaxZ+cameraWallOffset) - ballMinZ);
    } else {
        totalElapsedTime += timeDelta;
        if(hasLost) {
            if (mouseLeftReleased) {
                hasLost = false;
                hasStarted = false;
                currentKeyFrame = 0;
                previousKeyFrame = 0;
            }
        } else if (isPaused) {
            if (mouseRightReleased) {
                isPaused = false;
                if (options.enableMusic) {
                    sound->play();
                }
            }
        } else {
            gameElapsedTime += timeDelta;
            if (mouseRightReleased) {
                isPaused = true;
                if (options.enableMusic) {
                    sound->pause();
                }
            }
            // Get the timing for the beat of the song
            for (unsigned int i = currentKeyFrame; i < keyFrameTimeStamps.size(); i++) {
                if (gameElapsedTime < keyFrameTimeStamps.at(i)) {
                    continue;
                }
                currentKeyFrame = i;
            }

            jumpedToNextFrame = currentKeyFrame != previousKeyFrame;
            previousKeyFrame = currentKeyFrame;

            double frameStart = keyFrameTimeStamps.at(currentKeyFrame);
            double frameEnd = keyFrameTimeStamps.at(currentKeyFrame + 1); // Assumes last keyframe at infinity

            double elapsedTimeInFrame = gameElapsedTime - frameStart;
            double frameDuration = frameEnd - frameStart;
            double fractionFrameComplete = elapsedTimeInFrame / frameDuration;

            double ballYCoord;

            KeyFrameAction currentOrigin = keyFrameDirections.at(currentKeyFrame);
            KeyFrameAction currentDestination = keyFrameDirections.at(currentKeyFrame + 1);

            // Synchronize ball with music
            if (currentOrigin == BOTTOM && currentDestination == BOTTOM) {
                ballYCoord = ballBottomY;
            } else if (currentOrigin == TOP && currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance;
            } else if (currentDestination == BOTTOM) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * (1 - fractionFrameComplete);
            } else if (currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * fractionFrameComplete;
            }

            // Make ball move
            const float ballSpeed = 60.0f;
            ballPosition.x += timeDelta * ballSpeed * ballDirection.x;
            ballPosition.y = ballYCoord;
            ballPosition.z += timeDelta * ballSpeed * ballDirection.z;

            // Make ball bounce
            if (ballPosition.x < ballMinX) {
                ballPosition.x = ballMinX;
                ballDirection.x *= -1;
            } else if (ballPosition.x > ballMaxX) {
                ballPosition.x = ballMaxX;
                ballDirection.x *= -1;
            }
            if (ballPosition.z < ballMinZ) {
                ballPosition.z = ballMinZ;
                ballDirection.z *= -1;
            } else if (ballPosition.z > ballMaxZ) {
                ballPosition.z = ballMaxZ;
                ballDirection.z *= -1;
            }

            if(options.enableAutoplay) {
                padPositionX = 1-(ballPosition.x - ballMinX) / (ballMaxX - ballMinX);
                padPositionZ = 1-(ballPosition.z - ballMinZ) / ((ballMaxZ+cameraWallOffset) - ballMinZ);
            }

            // Check if the ball is hitting the pad when the ball is at the bottom.
            // If not, you just lost the game! (hehe)
            if (jumpedToNextFrame && currentOrigin == BOTTOM && currentDestination == TOP) {
                double padLeftX  = boxNode->position.x - (boxDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x);
                double padRightX = padLeftX + padDimensions.x;
                double padFrontZ = boxNode->position.z - (boxDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z);
                double padBackZ  = padFrontZ + padDimensions.z;

                if (   ballPosition.x < padLeftX
                    || ballPosition.x > padRightX
                    || ballPosition.z < padFrontZ
                    || ballPosition.z > padBackZ) {
                    hasLost = true;
                    if (options.enableMusic) {
                        sound->stop();
                        delete sound;
                    }
                }
            }
        }
    }

    glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);

    glm::vec3 cameraPosition = glm::vec3(0, 2, -20);

    // Some math to make the camera move in a nice way
    float lookRotation = -0.6 / (1 + exp(-5 * (padPositionX-0.5))) + 0.3;
    glm::mat4 cameraTransform = 
                    glm::rotate(0.3f + 0.2f * float(-padPositionZ*padPositionZ), glm::vec3(1, 0, 0)) *
                    glm::rotate(lookRotation, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);

    glm::mat4 VP = projection * cameraTransform;

    // Move and rotate various SceneNodes
    boxNode->position = { 0, -10, -80 };

    ballNode->position = ballPosition;
    ballNode->scale = glm::vec3(ballRadius);
    ballNode->rotation = { 0, totalElapsedTime*2, 0 };

    padNode->position  = { 
        boxNode->position.x - (boxDimensions.x/2) + (padDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x), 
        boxNode->position.y - (boxDimensions.y/2) + (padDimensions.y/2), 
        boxNode->position.z - (boxDimensions.z/2) + (padDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z)
    };

    // update uniforms that doesnt change that often (once per draw)
    glUniform3fv(10, 1, glm::value_ptr(cameraPosition));
    updateNodeTransformations(rootNode, VP, glm::mat4(1.0f));
    // 1c
    //glUniform3fv(7, NUM_POINT_LIGHTS, glm::value_ptr(pointLightPositions[0]));
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
    // node->currentTransformationMatrix = transformationThusFar * transformationMatrix;
    node->currentModelTransformationMatrix = transformationThusFar * transformationMatrix; // M
    node->currentTransformationMatrix = VP * node->currentModelTransformationMatrix; // MVP

    // 1d
    // Compute transpose of the inverse of the model matrix
    // Take the mat3 out of it
    // https://stackoverflow.com/questions/10879864/what-extractly-mat3a-mat4-matrix-statement-in-glsl-do
    node->currentNormalMatrix = glm::mat3(glm::transpose(glm::inverse(node->currentModelTransformationMatrix)));

    // push the ball node position to a uniform variable
    if (node == ballNode) {
        glm::vec4 pos = node->currentModelTransformationMatrix*glm::vec4(0.0f,0.0f,0.0f,1.0f);
        glUniform3fv(11, 1, glm::value_ptr(pos));
    }

    switch(node->nodeType) {
        case GEOMETRY: break;
        case GEOMETRY_NORMAL_MAPPED: break;
        case POINT_LIGHT:
            {
                glm::vec4 pos = node->currentModelTransformationMatrix*glm::vec4(0.0f,0.0f,0.0f,1.0f);
                glm::vec3 pos3 = glm::vec3(pos)/pos.w;  // Correct the length
                std::string uniformname = fmt::format("pointLights[{}].position", node->lightSourceID);
                GLint location = shader->getUniformFromName(uniformname);
                glUniform3fv(location, 1, glm::value_ptr(pos3));
            }
            break;
        case SPOT_LIGHT: break;
        case GROUP: break;
    }

    for(SceneNode* child : node->children) {
        updateNodeTransformations(child, VP, node->currentModelTransformationMatrix);
    }
}

void renderNode(SceneNode* node) {
    // MVP
    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
    // 1b  aka matrix M
    glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(node->currentModelTransformationMatrix));
    // 1d, pass normal matrix to the vertex shader
    glUniformMatrix3fv(5, 1, GL_FALSE, glm::value_ptr(node->currentNormalMatrix));

    switch(node->nodeType) {
    case GEOMETRY:
            if(node->vertexArrayObjectID != -1) {
                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        case GEOMETRY_NORMAL_MAPPED:
            {
                glUniform1i(7, 1); // useTexture
                glUniform1i(8, 1); // useNormalMap
                glUniform1i(9, 1); // useRoughnessMap
                glBindTextureUnit(1, node->textureID);
                glBindTextureUnit(2, node->normalMapTextureID);
                glBindTextureUnit(3, node->roughnessMapID);

                if(node->vertexArrayObjectID != -1) {
                    glBindVertexArray(node->vertexArrayObjectID);
                    glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
                }
                glUniform1i(7, 0);
                glUniform1i(8, 0);
                glUniform1i(9, 0);
            }
            break;
        case POINT_LIGHT: break;
        case SPOT_LIGHT: break;
        case GROUP: break;
    }

    for(SceneNode* child : node->children) {
        renderNode(child);
    }
}

void renderFrame(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0.0f, 0.0f, float(windowWidth), float(windowHeight));

    renderNode(rootNode);
}
