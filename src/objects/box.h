#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include "sceneGraph.hpp"
#ifndef GLOWBOX_BOX_H
#define GLOWBOX_BOX_H


class Box : public SceneNode {
private:
public:

    void generateNode(glm::vec3 dim, bool inverted) {
        Mesh m = cube(dim, glm::vec2(1.0f), false, inverted, glm::vec3(1.0f));
        unsigned int mVAO = generateBuffer(m);
        this->vertexArrayObjectID = (int) mVAO;
        this->VAOIndexCount = m.indices.size();
        this->nodeType = SceneNode::GEOMETRY;

        this->boundingBoxDimension = dim + glm::vec3(0.2f);
        this->hasBoundingBox = true;
    }

    Box(glm::vec3 dim, bool inverted) : SceneNode() {
        generateNode(dim, inverted);
    }
};


#endif //GLOWBOX_BOX_H
