#include "ship.h"
#include "utilities/shapes.h"
#include "utilities/glutils.h"
#include "sceneGraph.hpp"
#include <glm/gtc/random.hpp>
#include "laser.h"
#include <cmath>

// Sometimes i get mad at cpp
// Static variables
unsigned int Ship::total = 0;
unsigned int Ship::textureVaoId;
unsigned int Ship::textureIndicesCount;
bool Ship::textureCached = false;
std::vector<SceneNode*> Ship::attractors;
bool Ship::disableSafetyNet = false;

void Ship::generateShipNode() {
    if (!textureCached) {
        const glm::vec3 dboxDimensions(2, 3, 4);
        Mesh m = cube(dboxDimensions, glm::vec2(dboxDimensions.x, dboxDimensions.z), true);
        //Mesh m = generateTetrahedron(glm::vec3(1.0f));
        unsigned int mVAO = generateBuffer(m);
        Ship::textureVaoId = mVAO;
        Ship::textureIndicesCount = m.indices.size();
        Ship::textureCached = true;
    }
    //this->scale = glm::vec3(1.0f, 1.0f, 2.0f)*4.0f;
    this->vertexArrayObjectID = (int) Ship::textureVaoId;
    this->VAOIndexCount = Ship::textureIndicesCount;
    this->nodeType = SceneNode::GEOMETRY;

    this->position = glm::vec3(-4.0f, -49.0f, -100.0f);
    this->velocity = glm::ballRand(this->maxVelocity);

    this->material.baseColor = glm::vec3(0.0f, 0.0f, 1.0f);

    this->hasBoundingBox = true;
    //this->boundingBoxDimension = tetrahedronDim;
    this->boundingBoxDimension = glm::vec3(2,3,4)*2.0f;

    this->canUseBuffer = id % 2 == 0; // Create randomness
}

void Ship::updateShip(double deltaTime, std::vector<Ship*> &ships) {
    if (this->enabled) {
        this->acceleration = glm::vec3(0.0f);

        // Get all ships in proximity
        const std::vector<Ship *> closeShips = this->getShipsInRadius(ships);

        // Rule 1: Separation
        glm::vec3 separationForce = getSeparationForce(closeShips);

        // Rule 2: Alignment
        glm::vec3 alignmentForce = getAlignmentForce(closeShips);

        // Rule 3: Cohesion
        glm::vec3 cohesionForce = getCohesionForce(closeShips);

        this->acceleration += separationForce * this->weightSeparation;
        this->acceleration += alignmentForce * this->weightAlignment;
        this->acceleration += cohesionForce * this->weightCohesion;

        // Attaction force
        for (SceneNode* n : Ship::attractors) {
            glm::vec3 attractionForce = getForceFromVec(n->worldPos - this->worldPos);
            this->acceleration += attractionForce * this->weightAttraction;
        }

        // Anti collision force to avoid objects
        // TODO add weighted force to treat forces based on distance (if needed)
        Ray r = genRay(this->position, this->velocity);
        for (SceneNode *n : SceneNode::collisionObjects) {
            if (!n->hasBoundingBox || !n->enabled) continue;
            if (n->hasTinyBoundingBox && glm::length(n->worldPos - this->worldPos) + this->tinyBoundingBoxSize * 1.2f < this->perceptionRadius) continue;

            RayIntersection intersection = rayBoxIntersect(r, n->getBoundingBox());
            if (intersection.intersect && intersection.distance < this->perceptionCollisionRadius) {
                glm::vec3 antiCollisionForce = this->generateAntiCollisionForce(n);
                this->acceleration += antiCollisionForce * this->weightAntiCollision;
                this->material.baseColor = glm::vec3(1.0f, 1.0f, 1.0f);
            }
        }

        // Laser shooting mechanism
        this->laserMechanism(deltaTime, ships);

        // Check if collision with box, and move back inside, as not to just go away infinitely
        // Overwrites all other behaviors
        if (! Ship::disableSafetyNet) { this->barrierSafetyNet(); }

        // Calculate new velocity
        this->velocity = this->velocity + this->acceleration * (float) deltaTime; // v = v0 + at
        // Cap velocity over and under
        float speed = glm::length(this->velocity);
        glm::vec3 direction = this->velocity / speed; // Aka normalize
        speed = glm::clamp(speed, this->minVelocity, this->maxVelocity);
        this->velocity = speed * direction;

        // Update the ships position
        this->position += (float) deltaTime * this->velocity; // x = x0 + v*t

        // Set rotation
        // TODO select a component to calculate roll
        this->rotation = calcEulerAngles(direction);
    }

    // Must be done even if the node is disabled
    // Update lasers as they are sub-objects (todo move to graph tree directly?)
    for (int i = (int)lasers.size() - 1; i >= 0; i--) {
        Laser* l = lasers.at(i);
        l->update(deltaTime);

        // Clean up old lasers
        if (!l->enabled) {
            lasers.erase(lasers.begin() + i);
            delete l;
        }
    }
    //printShip();
}
void Ship::laserMechanism(double deltaTime, const std::vector<Ship *> &ships) {
    // Shooting laser mechanism
    this->laserRefraction -= (float) deltaTime;
    if (this->laserRefraction <= 0) {
        Ray r = genRay(this->position, this->velocity);
        for (SceneNode *n : ships) {
            if (this != n) {
                if (glm::length(n->position - this->position) < Ship::laserViewDistance) {
                    RayIntersection intersection = rayBoxIntersect(r, n->getBoundingBox());
                    if (intersection.intersect) {
                        this->generateLaser();
                        this->laserRefraction = Ship::minLaserRefraction;
                        break;
                    }
                }
            }
        }
    }
}

const int spherePoints = 40;
const float goldenRatio = (1.0f + std::pow(5.0f, 0.5f))/2.0f;
const float circleFactor = 0.7f;
glm::vec3 Ship::generateAntiCollisionForce(SceneNode *collisionObject) {
    // Assumes collision is "imminent"
    // Calculate possible escape-paths
    // https://stackoverflow.com/questions/9600801/evenly-distributing-n-points-on-a-sphere/44164075#44164075

    // To re-oriante the rays to be in the correct space
    glm::mat3 rotationMatrix = glm::mat3(
            glm::rotate(this->rotation.y, glm::vec3(0,1,0))
              * glm::rotate(this->rotation.x, glm::vec3(1,0,0))
              * glm::rotate(this->rotation.z, glm::vec3(0,0,1))
              );

    // Calculate it before, waisted resources vs readable code, or lazy evaluation instead?
    for (int i=0; i<spherePoints; i++) {
        float t = (float)i/((float)(spherePoints-1))*circleFactor; // 0 -> 1*fac, to not draw the entire sphere
        float inclination = std::acos(1.0f - 2.0f * t);
        float azimuth =  2.0f * M_PI * goldenRatio * i;

        float x = std::sin(inclination) * std::cos(azimuth);
        float y = std::sin(inclination) * std::sin(azimuth);
        float z = std::cos(inclination);

        glm::vec3 dir = glm::vec3(x, y, z);

        glm::vec3 dirWithFront = rotationMatrix*dir;

        Ray r = genRay(this->position, dirWithFront);
        RayIntersection intersection = rayBoxIntersect(r, collisionObject->getBoundingBox());
        if (!intersection.intersect || intersection.distance >= perceptionCollisionRadius) {
            // There is a 'safe' way to avoid the collision
            //lasers.push_back(new Laser(this->position, dirWithFront));
            return getForceFromVec(dirWithFront);
        }
    }
    return getForceFromVec(this->velocity * -1.0f);
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
glm::vec3 Ship::getForceFromVec(const glm::vec3 &vec, bool vecDiff) { // vecDiff is by default true
    if (glm::length(vec) < 0.2f) { // Ignore super tiny vectors and avoid div by 0 assumes allowed maxForce > val
        return vec;
    }
    glm::vec3 desiredVector = glm::normalize(vec) * this->maxVelocity;
    if (vecDiff) desiredVector -= this->velocity;
    return limitVector(desiredVector, this->maxForce);
}

std::vector<Ship*> Ship::getShipsInRadius(std::vector<Ship*> &ships) {
    // Half sampling to cpu load (if turned on)
    if (this->allowCpuLoadReduction && this->canUseBuffer) {
        this->canUseBuffer = false;
        return this->prevCloseShips;
    }
    std::vector<Ship*> returnList;
    for (Ship* ship2 : ships) {
        if (this != ship2 && ship2->enabled) {
            if (glm::length(this->position - ship2->position) <= this->perceptionRadius) {
                returnList.push_back(ship2);
            }
        }
    }
    if (this->allowCpuLoadReduction) {
        this->prevCloseShips = returnList;
        this->canUseBuffer = true;
    }
    return returnList;
}


//  x=0  => boxNode.x = 0
//  z box dim: 90/2 -80 = -35  -> -125,
//const glm::vec3 boxOffset(0, -10, -80);
const glm::vec3 boxOffset = glm::vec3(0, 0, 0);
//const glm::vec3 boxDimensions = glm::vec3(90, 90, 90)*2.0f;
const glm::vec3 boxDimensions(249.0f, 249.0f, 249.0f);
void Ship::barrierSafetyNet() {
    float x = this->position.x;
    float y = this->position.y;
    float z = this->position.z;

    float mf = this->maxForce;

    // -90 -> 90
    if (x > boxDimensions.x / 2 + boxOffset.x) this->acceleration.x = -mf;
    if (x < -boxDimensions.x / 2 + boxOffset.x) this->acceleration.x = mf;

    if (y > boxDimensions.y / 2 + boxOffset.y) this->acceleration.y = -mf;
    if (y < -boxDimensions.y / 2 + boxOffset.y) this->acceleration.y = mf;

    // -35 -> -125
    if (z > boxDimensions.z / 2 + boxOffset.z) this->acceleration.z = -mf;
    if (z < -boxDimensions.z / 2 + boxOffset.z) this->acceleration.z = mf;
}

void Ship::generateLaser() {
    this->lasers.push_back(new Laser(this->position, glm::normalize(this->velocity)));
}

glm::vec3 limitVector(const glm::vec3 &vec, float maxLength) {
    float length = glm::length(vec);
    if (length < 0.002f) return vec; // Avoid division by 0
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