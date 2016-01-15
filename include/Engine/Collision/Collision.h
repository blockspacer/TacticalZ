#ifndef Collision_h__
#define Collision_h__

//NOTE: Collision.h needs to be #included before <GLFW/glfw3.h>, 
//because Collision #includes "RawModel.h", which has "Texture.h", which has "OpenGL.h" which must be #included first
//or you will get "fatal error C1189: #error:  gl.h included before glew.h"

#include <vector>

#include "Core/Ray.h"
#include "Core/AABB.h"
#include "Engine/Rendering/RawModel.h"
#include "Core/Entity.h"

class World;
struct ComponentWrapper;

namespace Collision
{
//Return true if the ray hits the box.
bool RayAABBIntr(const Ray& ray, const AABB& box);
bool RayVsAABB(const Ray& ray, const AABB& box);
//Return true if the ray hits the box, also outputs distance from ray origin to intersection point in [outDistance].
bool RayVsAABB(const Ray& ray, const AABB& box, float& outDistance);

//Return true if the ray hits any of the triangles in the model. Stops checking when a hit is detected.
bool RayVsModel(const Ray& ray, 
    const std::vector<RawModel::Vertex>& modelVertices,
    const std::vector<unsigned int>& modelIndices);
//Return true if the ray hits any of the triangles in the model.
//Also returns the position of the intersection point. Will loop through all the whole model indices.
bool RayVsModel(const Ray& ray,
    const std::vector<RawModel::Vertex>& modelVertices,
    const std::vector<unsigned int>& modelIndices,
    glm::vec3& outHitPosition);
//Return true if the ray hits any of the triangles in the model.
//Also returns the distance from the ray origin to the closest 
//intersection point, and the barycentric u,v-coordinates. Will loop through all the whole model indices.
bool RayVsModel(const Ray& ray,
    const std::vector<RawModel::Vertex>& modelVertices,
    const std::vector<unsigned int>& modelIndices,
    float& outDistance,
    float& outUCoord,
    float& outVCoord);

//Return true if the boxes are intersecting.
bool AABBVsAABB(const AABB& a, const AABB& b);
//Return true if the boxes are intersecting.
//Also outputs the minimum translation that box [a] would need in order to resolve collision.
bool AABBVsAABB(const AABB& a, const AABB& b, glm::vec3& minimumTranslation);
bool IsSameBoxProbably(const AABB& first, const AABB& second, const float epsilon = 0.0001f);

//Returns true if the entity has a boundingbox. Outputs the aabb in [outBox].
bool GetEntityBox(World* world, EntityID entity, AABB& outBox, bool forceBoxFromModel = false);
bool GetEntityBox(World* world, ComponentWrapper& AABBComponent, AABB& outBox);

}

#endif