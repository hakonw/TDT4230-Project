#pragma once
#include "mesh.h"

Mesh cube(glm::vec3 scale = glm::vec3(1), glm::vec2 textureScale = glm::vec2(1), bool tilingTextures = false, bool inverted = false, glm::vec3 textureScale3d = glm::vec3(1));
Mesh generateSphere(float radius, int slices, int layers);
Mesh tetrahedrons(glm::vec3 scale = glm::vec3(1.0f));