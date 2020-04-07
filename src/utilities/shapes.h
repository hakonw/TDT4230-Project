#pragma once
#include "mesh.h"

const glm::vec3 tetrahedronDim = glm::vec3(1.0f, std::sqrt(6.0f) / 3.0f, std::sqrt(1.25f));

Mesh cube(glm::vec3 scale = glm::vec3(1), glm::vec2 textureScale = glm::vec2(1), bool tilingTextures = false, bool inverted = false, glm::vec3 textureScale3d = glm::vec3(1));
Mesh generateSphere(float radius, int slices, int layers);
Mesh generateTetrahedron(glm::vec3 scale = glm::vec3(1.0f));
Mesh generateUnitLine();