#pragma once

#include "sceneGraph.hpp"

class Ship : public SceneNode{
private:
    static unsigned int total;
    unsigned int id;

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

public:
    bool enabled = true;

    glm::vec3 acceleration = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 velocity = glm::vec3(1.0f, 1.0f, 1.0f);

    void generateShipNode();
    void updateShip(double deltaTime, std::vector<Ship*> &ships);
    void printShip();

    Ship() : SceneNode(), id(++total) {
        generateShipNode();
    }
};

glm::vec3 limitVector(const glm::vec3 &vec, float maxLength);