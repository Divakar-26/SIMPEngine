#pragma once

#include <AccelEngine/body.h>
#include <AccelEngine/shape.h>

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

namespace SIMPEngine::PhysicsBodies
{
    struct Options
    {
        float Mass = 1.0f;
        float Restitution = 0.3f;
        float StaticFriction = 0.6f;
        float DynamicFriction = 0.4f;
    };

    std::unique_ptr<AccelEngine::RigidBody> Circle(float radius, const Options &options = {});
    std::unique_ptr<AccelEngine::RigidBody> Box(glm::vec2 halfSize, const Options &options = {});
    std::unique_ptr<AccelEngine::RigidBody> Capsule(float halfLength, float radius, const Options &options = {});
    std::unique_ptr<AccelEngine::RigidBody> RegularPolygon(int sides, float radius, const Options &options = {});
    std::unique_ptr<AccelEngine::RigidBody> Polygon(const std::vector<glm::vec2> &points, float cornerRadius, const Options &options = {});
    std::unique_ptr<AccelEngine::RigidBody> FromParts(AccelEngine::ShapeType type, std::vector<AccelEngine::ConvexPart> parts, const Options &options, bool recenter);

    AccelEngine::ConvexPart MakePart(const std::vector<glm::vec2> &points, float radius);
    void ApplyOptions(AccelEngine::RigidBody &body, const Options &options);
    void Refresh(AccelEngine::RigidBody &body);

    const char *ShapeName(const AccelEngine::RigidBody &body);
    glm::vec2 ShapeExtents(const AccelEngine::RigidBody &body);
}
