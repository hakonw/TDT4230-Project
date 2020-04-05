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
    float perceptionRadius = 60.0f;
    float maxForce = 60.0f;

    float weightSeparation = 1.4f;
    float weightAlignment = 1.0f;
    float weightCohesion = 1.0f;

    std::vector<Ship*> getShipsInRadius(std::vector<Ship*> &ships);
    glm::vec3 getSeparationForce(const std::vector<Ship*> &closeShips);
    glm::vec3 getAlignmentForce(const std::vector<Ship*> &closeShips);
    glm::vec3 getCohesionForce(const std::vector<Ship*> &closeShips);
    glm::vec3 getForceFromVec(const glm::vec3 &vec, bool vecDiff=true);
    void barrierSafetyNet();

    std::vector<Laser*> lasers;

public:
    static SceneNode* laserGroup;

    glm::vec3 acceleration = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 velocity = glm::vec3(1.0f, 1.0f, 1.0f);

    void generateShipNode();
    void updateShip(double deltaTime, std::vector<Ship*> &ships);
    void generateLaser();

    unsigned int getIndependentChildrenSize() override {
        return lasers.size();
    }
    // TODO research if there is a "cheaper" way
    // Alternatively, have a list of casted pointers, manage both lists OR castback
    std::vector<SceneNode*> getIndependentChildren() override {
        return std::vector<SceneNode*>(lasers.begin(), lasers.end());
    }

    void printShip();

    Ship() : SceneNode(), id(++total) {
        generateShipNode();
    }
};

glm::vec3 limitVector(const glm::vec3 &vec, float maxLength);