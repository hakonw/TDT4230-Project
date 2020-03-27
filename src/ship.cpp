#include "ship.h"
#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include "sceneGraph.hpp"
#include <glm/gtc/random.hpp>
#include <glm/vec3.hpp>

// Sometimes i get mad at cpp
unsigned int Ship::total = 0;

void Ship::generateShipNode() {
    const glm::vec3 dboxDimensions(4, 3, 2);
    Mesh dbox = cube(dboxDimensions, glm::vec2(dboxDimensions.x, dboxDimensions.z), true);
    unsigned int dboxVAO = generateBuffer(dbox);
    this->sceneNode->vertexArrayObjectID = (int) dboxVAO;
    this->sceneNode->VAOIndexCount = dbox.indices.size();
    this->sceneNode->nodeType = GEOMETRY;

    this->sceneNode->position = glm::vec3(-4.0f, -49.0f, -100.0f);

    this->velocity.x = glm::linearRand(-1, 1);
    this->velocity.y = glm::linearRand(-1, 1);
    this->velocity.z = glm::linearRand(-1, 1);
    this->velocity *= this->maxVelocity;
}

void Ship::updateShip(double deltaTime, std::vector<Ship> &ships) {
    this->acceleration = glm::vec3(0.0f);

    // Get all ships in proximity
    const std::vector<Ship> closeShips = this->getShipsInRadius(ships);

    // Rule 1: Separation

    // Rule 2: Alignment
    glm::vec3 alignmentForce = getAlignmentForce(closeShips);


    this->acceleration += alignmentForce * this->weightAlignment;

    // Calculate new velocity
    this->velocity = this->velocity + this->acceleration * (float) deltaTime; // v = v0 + at
    // Cap velocity
    float speed = glm::length(this->velocity);
    glm::vec3 direction = this->velocity / speed; // Aka normalize
    speed = glm::clamp(speed, this->minVelocity, this->maxVelocity);
    this->velocity = speed * direction;

    // Check if collision with box
    this->ensureInsideBox();

    this->sceneNode->position += (float) deltaTime * this->velocity;
    printShip();


}

///
/// @param closeShips All ships within the perceptionRadius
/// @return Force for steering along the average
glm::vec3 Ship::getAlignmentForce(const std::vector<Ship> &closeShips) {
    if (closeShips.empty()) return glm::vec3(0.0f);

    glm::vec3 averageVelocity = glm::vec3(0.0f);
    for (Ship s : closeShips) {
        averageVelocity += s.velocity;
    }
    averageVelocity /= closeShips.size();

    glm::vec3 force = averageVelocity - this->velocity;
    return force;
}


std::vector<Ship> Ship::getShipsInRadius(std::vector<Ship> &ships) {
    std::vector<Ship> returnList;
    for (Ship ship2 : ships) {
        if (this != &ship2) { // TODO validate correct cpp
            if (glm::length(this->sceneNode->position - ship2.sceneNode->position) <= this->perceptionRadius) {
                returnList.push_back(ship2);
            }
        }
    }
    return returnList;
}


//glm::vec3 shipDirection = glm::vec3(1.0f, 1.0f, 1.0f);
//glm::vec3 shipDirection = glm::vec3(1.0f, 0.0f, 0.0f);
//   x=0  => boxNode.x = 0
//  z box dim: 90/2 -80 = -35  -> -125,
const glm::vec3 boxOffset(0, -10, -80);
const glm::vec3 boxDimensions(180, 90, 90);

void Ship::ensureInsideBox() {
    float x = this->sceneNode->position.x;
    float y = this->sceneNode->position.y;
    float z = this->sceneNode->position.z;

    // TODO fix so that it wont go halfway outside the object
    // -90 -> 90
    if (x > boxDimensions.x / 2 + boxOffset.x) this->velocity.x *= -1;
    if (x < -boxDimensions.x / 2 + boxOffset.x) this->velocity.x *= -1;

    //
    if (y > boxDimensions.y / 2 + boxOffset.y) this->velocity.y *= -1;
    if (y < -boxDimensions.y / 2 + boxOffset.y) this->velocity.y *= -1;

    // -35 -> -125
    if (z > boxDimensions.z / 2 + boxOffset.z) this->velocity.z *= -1;
    if (z < -boxDimensions.z / 2 + boxOffset.z) this->velocity.z *= -1;
}

void Ship::printShip() {
    printf(
            "Ship %i {\n"
            "    Location: (%f, %f, %f)\n"
            "    Velocity: (%f, %f, %f) - %f\n"
            "    Acceleration: (%f, %f, %f) - %f\n"
            "}\n",
            this->id,
            this->sceneNode->position.x, this->sceneNode->position.y, this->sceneNode->position.z,
            this->velocity.x, this->velocity.y, this->velocity.z, glm::length(this->velocity),
            this->acceleration.x, this->acceleration.y, this->acceleration.z, glm::length(this->acceleration)
            );
}
