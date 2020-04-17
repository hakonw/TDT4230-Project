#pragma once

#include <memory>
#include "sceneGraph.hpp"
#include "laser.h"

class Ship : public SceneNode{
    //typedef std::shared_ptr<Laser> LaserPtr; // Laser smart pointer alias
private:
    static unsigned int total;
    unsigned int id;

    static unsigned int textureVaoId;
    static unsigned int textureIndicesCount;
    static bool textureCached;

    float minVelocity = 50.0f;
    float maxVelocity = 80.0f;
    float perceptionRadius = 30.0f;
    float perceptionCollisionRadius = 45.0f;
    float maxForce = 80.0f;

    float weightSeparation = 1.4f;
    float weightAlignment = 1.0f;
    float weightCohesion = 0.7f;
    float weightAntiCollision = 3.0f;

    std::vector<Ship*> getShipsInRadius(std::vector<Ship*> &ships);
    glm::vec3 getSeparationForce(const std::vector<Ship*> &closeShips);
    glm::vec3 getAlignmentForce(const std::vector<Ship*> &closeShips);
    glm::vec3 getCohesionForce(const std::vector<Ship*> &closeShips);
    glm::vec3 generateAntiCollisionForce(const std::vector<SceneNode*> &collisionObjects);
    glm::vec3 getForceFromVec(const glm::vec3 &vec, bool vecDiff=true);
    void barrierSafetyNet();

    std::vector<Laser*> lasers;

    const float minLaserRefraction = 0.4f; // How often (min) can the ship shoot in seconds
    const float laserViewDistance = 100.0f;
    float laserRefraction = std::min(minLaserRefraction*5.0f, 5.0f); // Delay first laser

public:
    glm::vec3 acceleration = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 velocity = glm::vec3(1.0f, 1.0f, 1.0f);

    void generateShipNode();
    void updateShip(double deltaTime, std::vector<Ship*> &ships);
    void generateLaser();

    unsigned int getIndependentChildrenSize() override { return lasers.size(); }
    // TODO research if there is a "cheaper" way
    // Alternatively, have a list of casted pointers, manage both lists OR castback
    std::vector<SceneNode*> getIndependentChildren() override {
        return std::vector<SceneNode*>(lasers.begin(), lasers.end());
    }

    std::vector<SceneNode* > attractors;

    void printShip();

    Ship() : SceneNode(), id(++total) {
        generateShipNode();
    }
};

glm::vec3 limitVector(const glm::vec3 &vec, float maxLength);