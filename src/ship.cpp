#include "ship.h"
#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include "sceneGraph.hpp"
#include <glm/gtc/random.hpp>

void generateShipNode(SceneNode* node) {
    const glm::vec3 dboxDimensions(4, 3, 2);
    Mesh dbox = cube(dboxDimensions, glm::vec2(dboxDimensions.x, dboxDimensions.z), true);
    unsigned int dboxVAO  = generateBuffer(dbox);
    node->vertexArrayObjectID = (int) dboxVAO;
    node->VAOIndexCount = dbox.indices.size();
    node->nodeType = GEOMETRY;
    node->position = glm::vec3(-4.0f, -49.0f, -100.0f);

    // TODO make sure every ship gets the same speed
    node->momentum.x = glm::linearRand(-1, 1);
    node->momentum.y = glm::linearRand(-1, 1);
    node->momentum.z = glm::linearRand(-1, 1);
}

const float shipSpeed = 10.0f; // Max speed of a ship
//glm::vec3 shipDirection = glm::vec3(1.0f, 1.0f, 1.0f);
//glm::vec3 shipDirection = glm::vec3(1.0f, 0.0f, 0.0f);

//   x=0  => boxNode.x = 0
//  z box dim: 90/2 -80 = -35  -> -125,
const glm::vec3 boxOffset(0, -10, -80);
const glm::vec3 boxDimensions(180, 90, 90);
void updateMomentumInsideBox(SceneNode* ship) {
    float x = ship->position.x;
    float y = ship->position.y;
    float z = ship->position.z;

    // TODO fix so that it wont go halfway outside the object
    // -90 -> 90
    if (x > boxDimensions.x/2 + boxOffset.x) ship->momentum.x *= -1;
    if (x < -boxDimensions.x/2 + boxOffset.x) ship->momentum.x *= -1;

    //
    if (y > boxDimensions.y/2 + boxOffset.y) ship->momentum.y *= -1;
    if (y < -boxDimensions.y/2 + boxOffset.y) ship->momentum.y *= -1;

    // -35 -> -125
    if (z > boxDimensions.z/2 + boxOffset.z) ship->momentum.z *= -1;
    if (z < -boxDimensions.z/2 + boxOffset.z) ship->momentum.z *= -1;
}

void updateShipPosition(SceneNode* ship, float deltaTime) {

    // Check if collision with box
    updateMomentumInsideBox(ship);

    ship->position += deltaTime * shipSpeed * ship->momentum;
}