#include "CollisionDetector.h"
#include <algorithm>

namespace Client::World {

CollisionResult CollisionDetector::TestCylinderCylinder(const Cylinder& a, const Cylinder& b) {
    CollisionResult result;

    // Check height overlap
    float aMinZ = a.center.z;
    float aMaxZ = a.center.z + a.height;
    float bMinZ = b.center.z;
    float bMaxZ = b.center.z + b.height;

    if (aMaxZ <= bMinZ || aMinZ >= bMaxZ) {
        return result; // No collision on Z axis
    }

    // Check 2D distance
    Vector3 diff2D = {b.center.x - a.center.x, b.center.y - a.center.y, 0.0f};
    float distSq2D = diff2D.LengthSq();
    float sumRadius = a.radius + b.radius;
    float sumRadiusSq = sumRadius * sumRadius;

    if (distSq2D < sumRadiusSq) {
        result.hasCollision = true;
        float dist = std::sqrt(distSq2D);

        if (dist > 0.0001f) {
            result.contactNormal = diff2D / dist; // Pointing from a to b
            result.penetrationDepth = sumRadius - dist;
        } else {
            // Centers overlap, arbitrary normal
            result.contactNormal = {1.0f, 0.0f, 0.0f};
            result.penetrationDepth = sumRadius;
        }
    }

    return result;
}

CollisionResult CollisionDetector::TestCylinderAABB(const Cylinder& cyl, const AABB& aabb) {
    CollisionResult result;

    // Check height overlap
    float cylMinZ = cyl.center.z;
    float cylMaxZ = cyl.center.z + cyl.height;
    float aabbMinZ = aabb.min.z;
    float aabbMaxZ = aabb.max.z;

    if (cylMaxZ <= aabbMinZ || cylMinZ >= aabbMaxZ) {
        return result; // No collision on Z axis
    }

    // Find closest point on AABB in 2D (XY plane)
    Vector3 closestPoint;
    closestPoint.x = std::clamp(cyl.center.x, aabb.min.x, aabb.max.x);
    closestPoint.y = std::clamp(cyl.center.y, aabb.min.y, aabb.max.y);
    closestPoint.z = 0.0f;

    Vector3 cylCenter2D = {cyl.center.x, cyl.center.y, 0.0f};
    Vector3 diff2D = closestPoint - cylCenter2D;
    float distSq2D = diff2D.LengthSq();
    float radiusSq = cyl.radius * cyl.radius;

    if (distSq2D < radiusSq) {
        result.hasCollision = true;
        float dist = std::sqrt(distSq2D);

        // Check if Z axis penetration is smaller than XY penetration
        float distZMin = cylMaxZ - aabbMinZ;
        float distZMax = aabbMaxZ - cylMinZ;
        float minZPenetration = std::min(distZMin, distZMax);
        float xyPenetration = cyl.radius - dist;

        if (dist > 0.0001f) {
            if (minZPenetration < xyPenetration) {
                if (distZMin < distZMax) {
                    // Normal pointing from cylinder to AABB
                    result.contactNormal = {0.0f, 0.0f, 1.0f};
                    result.penetrationDepth = distZMin;
                } else {
                    result.contactNormal = {0.0f, 0.0f, -1.0f};
                    result.penetrationDepth = distZMax;
                }
            } else {
                result.contactNormal = diff2D / dist; // Pointing from cylinder to AABB
                result.penetrationDepth = xyPenetration;
            }
        } else {
            // Cylinder center is inside AABB
            // Find the closest face to push it out
            float distXMin = cyl.center.x - aabb.min.x;
            float distXMax = aabb.max.x - cyl.center.x;
            float distYMin = cyl.center.y - aabb.min.y;
            float distYMax = aabb.max.y - cyl.center.y;

            float minDist = std::min({distXMin, distXMax, distYMin, distYMax, distZMin, distZMax});

            if (minDist == distXMin) {
                result.contactNormal = {1.0f, 0.0f, 0.0f};
                result.penetrationDepth = cyl.radius + distXMin;
            } else if (minDist == distXMax) {
                result.contactNormal = {-1.0f, 0.0f, 0.0f};
                result.penetrationDepth = cyl.radius + distXMax;
            } else if (minDist == distYMin) {
                result.contactNormal = {0.0f, 1.0f, 0.0f};
                result.penetrationDepth = cyl.radius + distYMin;
            } else if (minDist == distYMax) {
                result.contactNormal = {0.0f, -1.0f, 0.0f};
                result.penetrationDepth = cyl.radius + distYMax;
            } else if (minDist == distZMin) {
                result.contactNormal = {0.0f, 0.0f, 1.0f};
                result.penetrationDepth = distZMin;
            } else {
                result.contactNormal = {0.0f, 0.0f, -1.0f};
                result.penetrationDepth = distZMax;
            }
        }
    } else {
        // Center of cylinder is outside the AABB in XY plane, but it might still overlap in Z only (if it was somehow not checked correctly, but height overlap was already checked). 
        // We actually need to check if the closest point is inside the cylinder radius.
        // The check `distSq2D < radiusSq` handles this, but it doesn't handle pure Z overlap if the closest point is within radius.
        // If distSq2D == 0, it falls into the `dist > 0.0001f` else branch which handles it.
        // But what if the cylinder is directly above/below the AABB and intersects only on Z?
        // In that case, distSq2D == 0 and it goes to the `else` block above.
        // Wait, if it's directly above, closestPoint will be the cylinder's X/Y.
        // So distSq2D will be 0. It handles it.
    }

    return result;
}

Vector3 CollisionDetector::CalculateSlideVector(const Vector3& velocity, const Vector3& normal) {
    float dotProduct = velocity.Dot(normal);
    // If moving away from the surface, no slide needed
    if (dotProduct >= 0.0f) {
        return velocity;
    }
    // Remove the velocity component along the normal
    return velocity - (normal * dotProduct);
}

Vector3 CollisionDetector::ResolveMovement(const Cylinder& player, const Vector3& velocity, 
                                           const std::vector<Cylinder>& actors, 
                                           const std::vector<AABB>& environment) {
    Vector3 finalVelocity = velocity;
    Cylinder movingPlayer = player;
    
    // Simulate multiple iterations for sliding
    const int maxIterations = 3;
    
    for (int i = 0; i < maxIterations; ++i) {
        Vector3 currentStepVelocity = finalVelocity;
        bool collided = false;
        Vector3 slideNormal = {0.0f, 0.0f, 0.0f};
        
        // Test against environment (AABBs) first as they are usually static
        for (const auto& aabb : environment) {
            Cylinder testPlayer = movingPlayer;
            testPlayer.center += currentStepVelocity;
            
            CollisionResult result = TestCylinderAABB(testPlayer, aabb);
            if (result.hasCollision) {
                collided = true;
                // Normal is pointing from player to AABB, so we need the opposite
                slideNormal = result.contactNormal * -1.0f;
                break;
            }
        }
        
        // Test against actors
        if (!collided) {
            for (const auto& actor : actors) {
                Cylinder testPlayer = movingPlayer;
                testPlayer.center += currentStepVelocity;
                
                CollisionResult result = TestCylinderCylinder(testPlayer, actor);
                if (result.hasCollision) {
                    collided = true;
                    // Normal is pointing from player to actor, so we need the opposite
                    slideNormal = result.contactNormal * -1.0f;
                    break;
                }
            }
        }
        
        if (collided) {
            // Calculate new velocity sliding along the obstacle
            finalVelocity = CalculateSlideVector(finalVelocity, slideNormal);
            // Move player up to the collision point before sliding in next iteration (simplification)
            // Ideally we'd move the player along the velocity vector up to the penetration depth, 
            // but for simple sliding, just updating the velocity for the full step is often used in basic character controllers.
            // If the velocity becomes too small, stop moving
            if (finalVelocity.LengthSq() < 0.0001f) {
                finalVelocity = {0.0f, 0.0f, 0.0f};
                break;
            }
        } else {
            // No collision in this step, movement is resolved
            break;
        }
    }
    
    return finalVelocity;
}

} // namespace Client::World
