#pragma once

#include <utilities/window.hpp>
#include "objects/sceneGraph.hpp"

void renderNode(SceneNode* node);
void updateNodeTransformations(SceneNode* node, glm::mat4 VP, glm::mat4 transformationThusFar);
void initGame(GLFWwindow* window, CommandLineOptions options);
void updateFrame(GLFWwindow* window);
void renderFrame(GLFWwindow* window);