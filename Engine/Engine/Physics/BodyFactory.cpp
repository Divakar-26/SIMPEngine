#include <Engine/Physics/BodyFactory.h>

#include <algorithm>
#include <cmath>

namespace SIMPEngine::PhysicsBodies
{
    using namespace AccelEngine;

    void ApplyOptions(RigidBody &body, const Options &options)
    {
        body.restitution = options.Restitution;
        body.staticFriction = options.StaticFriction;
        body.dynamicFriction = options.DynamicFriction;
        body.inverseMass = options.Mass > 0.0f ? 1.0f / options.Mass : 0.0f;
    }

    void Refresh(RigidBody &body)
    {
        body.calculateInertia();
        body.calculateDerivativeData();
    }

    namespace
    {
        std::unique_ptr<RigidBody> Make(const Options &options)
        {
            auto body = std::make_unique<RigidBody>();
            ApplyOptions(*body, options);
            return body;
        }
    }

    std::unique_ptr<RigidBody> Circle(float radius, const Options &options)
    {
        auto body = Make(options);
        body->shapeType = ShapeType::CIRCLE;
        body->circle.radius = std::max(radius, 0.01f);
        Refresh(*body);
        return body;
    }

    std::unique_ptr<RigidBody> Box(glm::vec2 halfSize, const Options &options)
    {
        auto body = Make(options);
        body->shapeType = ShapeType::AABB;
        body->aabb.halfSize = {std::max(halfSize.x, 0.01f), std::max(halfSize.y, 0.01f)};
        Refresh(*body);
        return body;
    }

    ConvexPart MakePart(const std::vector<glm::vec2> &points, float radius)
    {
        std::vector<Vector2> pts;
        pts.reserve(points.size());
        for (const glm::vec2 &p : points)
            pts.emplace_back(p.x, p.y);
        if (pts.size() == 1)
            return ConvexPart::circle(pts[0], radius);
        if (pts.size() == 2)
            return ConvexPart::capsule(pts[0], pts[1], radius);
        return ConvexPart::polygon(pts.data(), static_cast<int>(pts.size()), radius);
    }

    std::unique_ptr<RigidBody> FromParts(ShapeType type, std::vector<ConvexPart> parts, const Options &options, bool recenter)
    {
        parts.erase(std::remove_if(parts.begin(), parts.end(), [](const ConvexPart &p) { return p.count <= 0; }), parts.end());
        if (parts.empty())
            return nullptr;

        if (recenter)
        {
            real totalArea = 0.0f;
            Vector2 com(0, 0);
            for (const ConvexPart &p : parts)
            {
                real area, inertia;
                Vector2 centroid;
                p.massProperties(area, centroid, inertia);
                totalArea += area;
                com += centroid * area;
            }
            com = totalArea > 1e-9f ? com / totalArea : Vector2(0, 0);
            for (ConvexPart &p : parts)
            {
                for (int k = 0; k < p.count; ++k)
                    p.v[k] -= com;
                p.finalize();
            }
        }

        auto body = Make(options);
        body->shapeType = type;
        body->parts = std::move(parts);
        Refresh(*body);
        return body;
    }

    std::unique_ptr<RigidBody> Capsule(float halfLength, float radius, const Options &options)
    {
        halfLength = std::max(halfLength, 0.0f);
        return FromParts(ShapeType::CAPSULE, {ConvexPart::capsule({-halfLength, 0}, {halfLength, 0}, std::max(radius, 0.01f))}, options, true);
    }

    std::unique_ptr<RigidBody> RegularPolygon(int sides, float radius, const Options &options)
    {
        sides = std::clamp(sides, 3, static_cast<int>(kMaxPolygonVertices));
        std::vector<glm::vec2> points;
        for (int i = 0; i < sides; ++i)
        {
            const float angle = 6.28318531f * static_cast<float>(i) / static_cast<float>(sides);
            points.emplace_back(std::cos(angle) * radius, std::sin(angle) * radius);
        }
        return Polygon(points, 0.0f, options);
    }

    std::unique_ptr<RigidBody> Polygon(const std::vector<glm::vec2> &points, float cornerRadius, const Options &options)
    {
        if (points.size() < 3)
            return nullptr;
        ConvexPart part = MakePart(points, std::max(cornerRadius, 0.0f));
        if (part.count < 3)
            return nullptr;
        return FromParts(ShapeType::POLYGON, {part}, options, true);
    }

    const char *ShapeName(const RigidBody &body)
    {
        switch (body.shapeType)
        {
        case ShapeType::CIRCLE:
            return "circle";
        case ShapeType::AABB:
            return "box";
        case ShapeType::POLYGON:
            return "polygon";
        case ShapeType::CAPSULE:
            return "capsule";
        case ShapeType::COMPOUND:
            return "compound";
        }
        return "unknown";
    }

    glm::vec2 ShapeExtents(const RigidBody &body)
    {
        switch (body.shapeType)
        {
        case ShapeType::CIRCLE:
            return {body.circle.radius * 2.0f, body.circle.radius * 2.0f};
        case ShapeType::AABB:
            return {body.aabb.halfSize.x * 2.0f, body.aabb.halfSize.y * 2.0f};
        default:
            return {body.localBoundsHalf.x * 2.0f, body.localBoundsHalf.y * 2.0f};
        }
    }
}
