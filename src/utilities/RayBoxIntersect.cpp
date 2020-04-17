#include "RayBoxIntersect.h"
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>


Ray genRay(glm::vec3 pos, glm::vec3 dir) {
    return Ray{
        pos,
        dir,
        1.0f/dir
    };
}

// Can return v1, v2 for AABB, but then it is not strictly correct
BoundingBox genBoundingBox(glm::vec3 position, glm::vec3 dimension, glm::vec3 scale) {
    glm::vec3 v1 = position - (dimension*scale/2.0f);
    glm::vec3 v2 = position + (dimension*scale/2.0f);

    return BoundingBox {
            v1,
            v2
    };
    /*
     // This is more correct
     // But as orientiation isnt important, spare a few cycles, even tho it should be branchless and fast
    glm::vec3 vmin;
    glm::vec3 vmax;

    vmin.x = dmnsn_min(v1.x, v2.x);
    vmin.y = dmnsn_min(v1.y, v2.y);
    vmin.z = dmnsn_min(v1.z, v2.z);

    vmax.x = dmnsn_max(v1.x, v2.x);
    vmax.y = dmnsn_max(v1.y, v2.y);
    vmax.z = dmnsn_max(v1.z, v2.z);

    return BoundingBox {
        vmin,
        vmax
    };
     */
}

RayIntersection rayBoxIntersect(Ray ray, BoundingBox boundingBox) {
    float tx1 = (boundingBox.min.x - ray.rayPos.x) * ray.rayFrac.x;
    float tx2 = (boundingBox.max.x - ray.rayPos.x) * ray.rayFrac.x;

    float tmin = dmnsn_min(tx1, tx2); // Closest point
    float tmax = dmnsn_max(tx1, tx2); // Furthest away point

    float ty1 = (boundingBox.min.y - ray.rayPos.y) * ray.rayFrac.y;
    float ty2 = (boundingBox.max.y - ray.rayPos.y) * ray.rayFrac.y;

    tmin = dmnsn_max(tmin, dmnsn_min(ty1, ty2)); // Furthest away of nearest planes
    tmax = dmnsn_min(tmax, dmnsn_max(ty1, ty2)); // closest of furthest away planes

    float tz1 = (boundingBox.min.z - ray.rayPos.z) * ray.rayFrac.z;
    float tz2 = (boundingBox.max.z - ray.rayPos.z) * ray.rayFrac.z;

    tmin = dmnsn_max(tmin, dmnsn_min(tz1, tz2));
    tmax = dmnsn_min(tmax, dmnsn_max(tz1, tz2));

    // tmax < 0    means that ray intersects AABB behind (negative dir) (t = tmax)
    // tmin > tmax means ray does not intersect AABB (t = tmax = inf?)
    // tmin < 0    means ray origin is inside of AABB, and first intersect is at tmax (t = tmax)
    // tmin is the distance (in vectors) to the intersect (t = tmin)
    RayIntersection intersection{};

    // So much for being branchless
    if (tmax < 0) { // Intersection on wrong side
        intersection.intersect = false;
        intersection.t = tmax;
    }

    if (tmin > tmax) { // No intersection
        intersection.intersect = false;
        intersection.t = tmax;
    }

    if (tmax >= tmin && tmax >= 0) { // Intersect
        intersection.intersect = true;
        if (tmin < 0) { // inside
            intersection.t = tmax;
        } else {
            intersection.t = tmin;
        }
        intersection.distance = glm::length(intersection.t * ray.rayDir);
    }
    return intersection;
}
