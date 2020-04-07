#ifndef GLOWBOX_RAYBOXINTERSECT_H
#define GLOWBOX_RAYBOXINTERSECT_H

#include <glm/vec3.hpp>

struct BoundingBox{
    glm::vec3 min;
    glm::vec3 max;
};

struct Ray{
    glm::vec3 rayPos;
    glm::vec3 rayDir;
    glm::vec3 rayFrac;
};

Ray genRay(glm::vec3 pos, glm::vec3 dir);
BoundingBox genBoundingBox(glm::vec3 position, glm::vec3 dimension, glm::vec3 scale);
bool rayBoxIntersect(Ray ray, BoundingBox boundingBox);

// https://distantsoulsdev.blogspot.com/2013/03/adventures-in-branchless-min-max-with.html
// https://stackoverflow.com/questions/40196817/what-is-the-instruction-that-gives-branchless-fp-min-and-max-on-x86
inline float dmnsn_min(float a, float b) {
    return a < b ? a : b;
}

inline float dmnsn_max(float a, float b) {
    return a > b ? a : b;
}

#endif //GLOWBOX_RAYBOXINTERSECT_H
