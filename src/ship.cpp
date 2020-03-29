#include "ship.h"
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include "sceneGraph.hpp"
#include <glm/gtc/random.hpp>
#include <glm/vec3.hpp>

// Sometimes i get mad at cpp
// Static variables
unsigned int Ship::total = 0;
unsigned int Ship::textureVaoId;
unsigned int Ship::textureIndicesCount;
bool Ship::textureCached = false;

void Ship::generateShipNode() {
    if (!textureCached) {
        const glm::vec3 dboxDimensions(4, 3, 2);
        Mesh m = cube(dboxDimensions, glm::vec2(dboxDimensions.x, dboxDimensions.z), true);
        //Mesh m = tetrahedrons(glm::vec3(4.0f, 6.0f, 4.0f));
        unsigned int mVAO = generateBuffer(m);
        Ship::textureVaoId = mVAO;
        Ship::textureIndicesCount = m.indices.size();
        Ship::textureCached = true;
    }
    this->vertexArrayObjectID = (int) Ship::textureVaoId;
    this->VAOIndexCount = Ship::textureIndicesCount;
    this->nodeType = SceneNode::GEOMETRY;

    this->position = glm::vec3(-4.0f, -49.0f, -100.0f);
    this->velocity = glm::ballRand(this->maxVelocity);
}

void Ship::updateShip(double deltaTime, std::vector<Ship*> &ships) {
    this->acceleration = glm::vec3(0.0f);

    // Get all ships in proximity
    const std::vector<Ship*> closeShips = this->getShipsInRadius(ships);

    // Rule 1: Separation
    glm::vec3 separationForce = getSeparationForce(closeShips);

    // Rule 2: Alignment
    glm::vec3 alignmentForce = getAlignmentForce(closeShips);

    // Rule 3: Cohesion
    glm::vec3 cohesionForce = getCohesionForce(closeShips);

    this->acceleration += separationForce * this->weightSeparation;
    this->acceleration +=  alignmentForce * this->weightAlignment;
    this->acceleration +=   cohesionForce * this->weightCohesion;


    // Check if collision with box, and move back inside, as not to just go away infinitely
    // Overwrites all other behaviors
    this->barrierSafetyNet();

    // Calculate new velocity
    this->velocity = this->velocity + this->acceleration * (float) deltaTime; // v = v0 + at
    // Cap velocity over and under
    float speed = glm::length(this->velocity);
    glm::vec3 direction = this->velocity / speed; // Aka normalize
    speed = glm::clamp(speed, this->minVelocity, this->maxVelocity);
    this->velocity = speed * direction;

    // Update the ships possition
    this->position += (float) deltaTime * this->velocity; // x = x0 + v*t

    // Set rotation
    this->rotation = normalize(direction);

    //printShip();
}

/// Calculate the separation force with a (linear) inverse proportional factor
/// @param closeShips all ships within the perceptionRadius
/// @return force for steering away from all neighbours
glm::vec3 Ship::getSeparationForce(const std::vector<Ship*> &closeShips) {
    if (closeShips.empty()) return glm::vec3(0.0f);

    // Sum of forces, between 0 and 1
    glm::vec3 separationForce = glm::vec3(0.0f);
    for (const Ship* s : closeShips) {
        // vec from s to this
        glm::vec3 sepF = this->position - s->position;
        float l = glm::length(sepF);

        if (l == 0) { // if they get inside of each other, use random vector
            separationForce += glm::ballRand(2.0f);
        } else {
            separationForce += sepF / l;
        }
    }
    return getForceFromVec(separationForce, false);
}

/// Calculate the alignment force
/// @param closeShips all ships within the perceptionRadius
/// @return force for steering along the average velocity
glm::vec3 Ship::getAlignmentForce(const std::vector<Ship*> &closeShips) {
    if (closeShips.empty()) return glm::vec3(0.0f);

    glm::vec3 averageVelocity = glm::vec3(0.0f);
    for (const Ship* s : closeShips) {
        averageVelocity += s->velocity;
    }
    //averageVelocity /= closeShips.size(); // Not needed, but nice for visualization of the math
    return getForceFromVec(averageVelocity);
}

/// Calculate the cohesion force
/// @param closeShips all ships within the perceptionRadius
/// @return force for steering towards the average position
glm::vec3 Ship::getCohesionForce(const std::vector<Ship*> &closeShips) { // TODO weighted?
    if (closeShips.empty()) return glm::vec3(0.0f);

    glm::vec3 centerOfFlock = glm::vec3(0.0f);
    for (Ship* s : closeShips) {
        centerOfFlock += s->position;
    }
    centerOfFlock /= closeShips.size();

    // Vector from ship to center of flock
    glm::vec3 shipToFlock = centerOfFlock - this->position;
    return getForceFromVec(shipToFlock);
}

/// Transform steering-direction to a desired velocity vector, which we can use as a force
/// @param vec vector in the desired location
/// @param vecDiff to subtract the currents ship velocity to get the desired force, used by separation-force
/// @return steering force
glm::vec3 Ship::getForceFromVec(const glm::vec3 &vec, bool vecDiff) {
    if (glm::length(vec) < 0.2f) { // Ignore super tiny vectors and avoid div by 0 assumes allowed maxForce > val
        return vec;
    }
    glm::vec3 desiredVector = glm::normalize(vec) * this->maxVelocity;
    if (vecDiff) desiredVector -= this->velocity;
    return limitVector(desiredVector, this->maxForce);
}

std::vector<Ship*> Ship::getShipsInRadius(std::vector<Ship*> &ships) {
    std::vector<Ship*> returnList;
    for (Ship* ship2 : ships) {
        if (this != ship2) {
            if (glm::length(this->position - ship2->position) <= this->perceptionRadius) {
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

void Ship::barrierSafetyNet() {
    float x = this->position.x;
    float y = this->position.y;
    float z = this->position.z;

    float mf = this->maxForce;

    // TODO fix so that it wont go halfway outside the object
    // -90 -> 90
    if (x > boxDimensions.x / 2 + boxOffset.x) this->acceleration.x = -mf;
    if (x < -boxDimensions.x / 2 + boxOffset.x) this->acceleration.x = mf;

    //
    if (y > boxDimensions.y / 2 + boxOffset.y) this->acceleration.y = -mf;
    if (y < -boxDimensions.y / 2 + boxOffset.y) this->acceleration.y = mf;

    // -35 -> -125
    if (z > boxDimensions.z / 2 + boxOffset.z) this->acceleration.z = -mf;
    if (z < -boxDimensions.z / 2 + boxOffset.z) this->acceleration.z = mf;
}

glm::vec3 limitVector(const glm::vec3 &vec, float maxLength) {
    float length = glm::length(vec);
    if (length == 0.0f) return vec; // Avoid division by 0
    glm::vec3 direction = vec/length;
    length = std::min(length, maxLength);
    return length * direction;
}

void Ship::printShip() {
    printf(
            "Ship %i {\n"
            "    Location: (%f, %f, %f)\n"
            "    Velocity: (%f, %f, %f) - %f\n"
            "    Acceleration: (%f, %f, %f) - %f\n"
            "}\n",
            this->id,
            this->position.x, this->position.y, this->position.z,
            this->velocity.x, this->velocity.y, this->velocity.z, glm::length(this->velocity),
            this->acceleration.x, this->acceleration.y, this->acceleration.z, glm::length(this->acceleration)
            );
}