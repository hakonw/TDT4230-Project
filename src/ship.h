#pragma once

#include "sceneGraph.hpp"

class Ship {
public:
    static unsigned int total;
    const unsigned int id;
    static std::vector<Ship> &ships;

    float minVelocity = 10.0f;
    float maxVelocity = 30.0f;
    float perceptionRadius = 60.0f;
    float maxForce = 5.0f;

    float weightSeparation = 1.0f;
    float weightAlignment = 1.0f;
    float weightCohesion = 1.0f;

    SceneNode *sceneNode;

    glm::vec3 acceleration = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 velocity = glm::vec3(1.0f, 1.0f, 1.0f);
    //glm::vec3 *position = &sceneNode->position;
    //glm::vec3 *rotation = &sceneNode->rotation;

    void generateShipNode();
    void updateShip(double deltaTime, std::vector<Ship> &ships);
    std::vector<Ship> getShipsInRadius(std::vector<Ship> &ships);
    void ensureInsideBox();
    glm::vec3 getAlignmentForce(const std::vector<Ship> &closeShips);
    void printShip();

    Ship() : id(++total){
        sceneNode = createSceneNode();
        generateShipNode();
    }
};
