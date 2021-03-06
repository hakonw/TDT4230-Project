#pragma once

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <vector>
#include <cstdio>
#include "utilities/RayBoxIntersect.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

struct Material {
    glm::vec3 baseColor = glm::vec3{1.0f, 1.0f, 1.0f};
    float shininess = 32;
};


class SceneNode {
private:

public:
    enum SceneNodeType {
        GEOMETRY, POINT_LIGHT, GEOMETRY_NORMAL_MAPPED, GROUP, LINE
    };

    SceneNode() {
        position = glm::vec3(0, 0, 0);
        rotation = glm::vec3(0, 0, 0);
        scale = glm::vec3(1, 1, 1);

        referencePoint = glm::vec3(0, 0, 0);
        vertexArrayObjectID = -1;
        VAOIndexCount = 0;

        enabled = true;
        nodeType = GEOMETRY;
    }

    SceneNode(SceneNodeType type) : SceneNode() {
        nodeType = type;
    }

    // To draw or not to draw
    bool enabled;

    // A list of all children that belong to this node.
    // For instance, in case of the scene graph of a human body shown in the assignment text, the "Upper Torso" node would contain the "Left Arm", "Right Arm", "Head" and "Lower Torso" nodes in its list of children.
    std::vector<SceneNode *> children;

    // List of children following inheriting movement from parent, but is still a child of this node.
    // Must be overwritten by other classes
    virtual unsigned int getIndependentChildrenSize() {
        return 0;
    }

    virtual std::vector<SceneNode *> getIndependentChildren() {
        return std::vector<SceneNode *>();
    }

    // Bounding box fields
    bool hasBoundingBox = false;
    glm::vec3 boundingBoxDimension;
    virtual BoundingBox getBoundingBox() { return genBoundingBox(this->position, this->boundingBoxDimension, this->scale); }
    static std::vector<SceneNode *> collisionObjects; // List of all collision objects
    bool hasTinyBoundingBox = false;
    float tinyBoundingBoxSize = 10.0f;

    // The node's position and rotation relative to its parent
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    glm::vec3 worldPos;

    bool staticRefScaleRot = false;
    glm::mat4 refScaleRot;

    void setStaticMat() {
        this->refScaleRot =
            glm::translate(this->referencePoint)
            * glm::rotate(this->rotation.y, glm::vec3(0,1,0))
            * glm::rotate(this->rotation.x, glm::vec3(1,0,0))
            * glm::rotate(this->rotation.z, glm::vec3(0,0,1))
            * glm::scale(this->scale)
            * glm::translate(-this->referencePoint);
        staticRefScaleRot = true;
    }

    Material material;

    // A transformation matrix representing the transformation of the node's location relative to its parent. This matrix is updated every frame.
    glm::mat4 currentTransformationMatrix;

    // Model transformation
    glm::mat4 currentModelTransformationMatrix;

    // Normal matrix
    glm::mat3 currentNormalMatrix;

    // The location of the node's reference point
    glm::vec3 referencePoint;

    // Mesh meta data object
    // The ID of the VAO containing the "appearance" of this SceneNode.
    int vertexArrayObjectID;
    unsigned int VAOIndexCount;

    // Node type is used to determine how to handle the contents of a node
    SceneNodeType nodeType;

    // OtherID, used for identifying light
    int lightSourceID;

    // texture ID, Assigment 2 task 1h
    unsigned int textureID;
    unsigned int normalMapTextureID;
    unsigned int roughnessMapID;

    unsigned int ignoreLight = 0;


    void addChild(SceneNode *child) {
        this->children.push_back(child);
    }

    // Pretty prints the current values of a SceneNode instance to stdout
    void printNode() {
        printf(
                "SceneNode {\n"
                "    Child count: %i\n"
                "    Rotation: (%f, %f, %f)\n"
                "    Location: (%f, %f, %f)\n"
                "    Reference point: (%f, %f, %f)\n"
                "    VAO ID: %i\n"
                "    Type: %i\n"
                "}\n",
                int(SceneNode::children.size()),
                SceneNode::rotation.x, SceneNode::rotation.y, SceneNode::rotation.z,
                SceneNode::position.x, SceneNode::position.y, SceneNode::position.z,
                SceneNode::referencePoint.x, SceneNode::referencePoint.y, SceneNode::referencePoint.z,
                SceneNode::vertexArrayObjectID, SceneNode::nodeType);
    }

    int totalChildren() {
        int count = SceneNode::children.size();
        for (SceneNode *child : SceneNode::children) {
            count += child->totalChildren();
        }
        return count;
    }

    // AS long as the node isnt used (as with multi threading), deleting is safe
    // Cleaning should be done in a single thread or when there is no children for safety
    virtual ~SceneNode() {
        for (SceneNode* node : this->getIndependentChildren() ) {
                delete node;
                node = nullptr;
        }

        for (SceneNode* node : children) {
                delete node;
                node = nullptr;
        }
    };
};

inline glm::vec3 calcEulerAngles(const glm::vec3 &direction) {
    glm::vec3 dir = normalize(direction);
    float pitch = std::asin(-dir.y);
    float yaw = std::atan2(dir.x, dir.z);
    return glm::vec3(pitch, yaw, 0.0f);
}