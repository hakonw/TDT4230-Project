#ifndef GLOWBOX_LASER_H
#define GLOWBOX_LASER_H

#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include "sceneGraph.hpp"

class Laser : public SceneNode {
private:
    static unsigned int textureVaoId;
    static unsigned int textureIndicesCount;
    static bool textureCached;

    const float length = 10.0f;
    const float velocityMagnitude = 160.0f;
    const float lifeTime = 2.0f; // Lifetime in seconds before despawning
    float totalTime = 0;

    glm::vec3 direction{};


public:
    void generateNode(glm::vec3 pos, glm::vec3 dir) {
        // Cache texture
        if (!textureCached) {
            Mesh m = generateUnitLine();
            unsigned int mVAO = generateBuffer(m);
            Laser::textureVaoId = mVAO;
            Laser::textureIndicesCount = m.indices.size();
            Laser::textureCached = true;
        }
        this->vertexArrayObjectID = (int) Laser::textureVaoId;
        this->VAOIndexCount = Laser::textureIndicesCount;
        this->nodeType = SceneNode::LINE;

        this->position = pos;
        this->direction = glm::normalize(dir);
        this->rotation = calcEulerAngles(this->direction);
        this->scale = glm::vec3(this->length);

        this->material.baseColor = glm::vec3(1.0f, 0.0f, 0.0f);
        this->ignoreLight = true;
    }

    void update(double deltaTime) {
        totalTime += (float) deltaTime;
        if (totalTime < lifeTime) {
            this->position += (float) deltaTime * (this->direction * this->velocityMagnitude);
        } else {
            // Ask to be deleted
            // TODO Ask to go back into a pool of lasers for later use?
            this->enabled = false;
        }

    }

    Laser(glm::vec3 pos, glm::vec3 dir) : SceneNode() {
        generateNode(pos, dir);
    }
};

#endif //GLOWBOX_LASER_H
