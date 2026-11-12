#include <Engine/Rendering/Renderer.h>
#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Rendering/SvgTextureCache.h>
#include <Engine/Rendering/SVGDocument.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace SIMPEngine
{
    int Renderer::m_WindowHeight = 0;
    int Renderer::m_WindowWidth = 0;

    std::unique_ptr<RenderingAPI> Renderer::s_RenderingAPI = nullptr;
    glm::mat4 Renderer::s_ViewMatrix = glm::mat4(1.0f);
    BlendMode Renderer::s_BlendMode = BlendMode::Alpha;
    glm::vec4 Renderer::s_ColorMultiplier{1.0f};
    glm::mat4 Renderer::s_CullViewProjection{0.0f};
    glm::vec2 Renderer::s_CullMin{0.0f};
    glm::vec2 Renderer::s_CullMax{0.0f};

    namespace
    {
        glm::vec4 ToVec4(SDL_Color c)
        {
            return {static_cast<float>(c.r) / 255.0f, static_cast<float>(c.g) / 255.0f, static_cast<float>(c.b) / 255.0f, static_cast<float>(c.a) / 255.0f};
        }
    }

    RenderingAPI *Renderer::GetAPI()
    {
        return s_RenderingAPI.get();
    }

    void Renderer::Init(std::unique_ptr<RenderingAPI> api, int w, int h)
    {
        s_RenderingAPI = std::move(api);
        m_WindowWidth = w;
        m_WindowHeight = h;
        s_RenderingAPI->ResizeViewport(w, h);
        s_RenderingAPI->Init();
    }

    void Renderer::SetWindowSize(int w, int h)
    {
        m_WindowWidth = std::max(0, w);
        m_WindowHeight = std::max(0, h);
    }

    void Renderer::Shutdown()
    {
        if (s_RenderingAPI)
            s_RenderingAPI->Flush();
        s_RenderingAPI.reset();
        s_ViewMatrix = glm::mat4(1.0f);
        m_WindowWidth = 0;
        m_WindowHeight = 0;
    }

    void Renderer::SetClearColor(float r, float g, float b, float a)
    {
        s_RenderingAPI->SetClearColor(r, g, b, a);
    }

    void Renderer::Clear()
    {
        s_RenderingAPI->Clear();
    }

    void Renderer::BeginScene(const glm::mat4 &view, const glm::mat4 &projection)
    {
        SetProjectionMatrix(projection);
        SetViewMatrix(view);
    }

    void Renderer::EndScene()
    {
        s_RenderingAPI->Flush();
    }

    void Renderer::BeginRenderTarget(Framebuffer &target)
    {
        s_RenderingAPI->BindRenderTarget(&target);
    }

    void Renderer::EndRenderTarget()
    {
        s_RenderingAPI->BindRenderTarget(nullptr);
    }

    void Renderer::Recover()
    {
        if (!s_RenderingAPI)
            return;
        s_RenderingAPI->BindRenderTarget(nullptr);
        s_BlendMode = BlendMode::Alpha;
        s_ColorMultiplier = glm::vec4(1.0f);
    }

    void Renderer::SubmitPrimitive(Primitive &primitive)
    {
        float radius = glm::length(primitive.HalfSize + glm::vec2(primitive.Padding));
        if (!IsVisible(primitive.Center, glm::vec2(radius)))
            return;
        primitive.Color *= s_ColorMultiplier;
        s_RenderingAPI->Submit(primitive);
    }

    void Renderer::DrawTriangles(const glm::vec2 *positions, const glm::vec4 *colors, size_t vertexCount, const uint32_t *indices, size_t indexCount, float zIndex)
    {
        if (!positions || !colors || !indices || vertexCount == 0 || indexCount < 3)
            return;
        glm::vec2 min = positions[0];
        glm::vec2 max = positions[0];
        for (size_t i = 1; i < vertexCount; ++i)
        {
            min = glm::min(min, positions[i]);
            max = glm::max(max, positions[i]);
        }
        if (!IsVisible((min + max) * 0.5f, (max - min) * 0.5f))
            return;

        if (s_ColorMultiplier == glm::vec4(1.0f))
        {
            s_RenderingAPI->SubmitMesh(positions, colors, indices, indexCount, zIndex, s_BlendMode);
            return;
        }
        thread_local std::vector<glm::vec4> tinted;
        tinted.resize(vertexCount);
        for (size_t i = 0; i < vertexCount; ++i)
            tinted[i] = colors[i] * s_ColorMultiplier;
        s_RenderingAPI->SubmitMesh(positions, tinted.data(), indices, indexCount, zIndex, s_BlendMode);
    }

    void Renderer::SetColorMultiplier(const glm::vec4 &multiplier)
    {
        s_ColorMultiplier = multiplier;
    }

    const glm::vec4 &Renderer::GetColorMultiplier()
    {
        return s_ColorMultiplier;
    }

    void Renderer::SetBlendMode(BlendMode mode)
    {
        s_BlendMode = mode;
    }

    BlendMode Renderer::GetBlendMode()
    {
        return s_BlendMode;
    }

    void Renderer::GetVisibleBounds(glm::vec2 &min, glm::vec2 &max)
    {
        glm::mat4 viewProjection = s_RenderingAPI->GetProjectionMatrix() * s_ViewMatrix;
        if (viewProjection != s_CullViewProjection)
        {
            s_CullViewProjection = viewProjection;
            glm::mat4 inverse = glm::inverse(viewProjection);
            const glm::vec2 corners[4] = {{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}};
            for (int i = 0; i < 4; ++i)
            {
                glm::vec4 w = inverse * glm::vec4(corners[i], 0.0f, 1.0f);
                glm::vec2 p(w.x, w.y);
                s_CullMin = i == 0 ? p : glm::min(s_CullMin, p);
                s_CullMax = i == 0 ? p : glm::max(s_CullMax, p);
            }
        }
        min = s_CullMin;
        max = s_CullMax;
    }

    bool Renderer::IsVisible(const glm::vec2 &center, const glm::vec2 &halfExtent)
    {
        glm::vec2 min, max;
        GetVisibleBounds(min, max);
        glm::vec2 extent = glm::abs(halfExtent);
        return center.x + extent.x >= min.x && center.x - extent.x <= max.x &&
               center.y + extent.y >= min.y && center.y - extent.y <= max.y;
    }

    float Renderer::GetWorldPerPixel()
    {
        const glm::mat4 &proj = s_RenderingAPI->GetProjectionMatrix();
        glm::vec2 size = s_RenderingAPI->GetViewportSize();
        float sy = std::abs(proj[1][1]);
        if (sy <= 0.0f || size.y <= 0.0f)
            return 1.0f;
        return 2.0f / (sy * size.y);
    }

    void Renderer::DrawQuad(float x, float y, float width, float height, float rotation, SDL_Color color, bool fill, float zIndex)
    {
        if (!fill)
        {
            DrawRect(x, y, width, height, GetWorldPerPixel(), color, rotation, zIndex);
            return;
        }

        Primitive p;
        p.Center = {x, y};
        p.HalfSize = {width * 0.5f, height * 0.5f};
        p.Rotation = glm::radians(rotation);
        p.Z = zIndex;
        p.Color = ToVec4(color);
        p.Blend = s_BlendMode;
        SubmitPrimitive(p);
    }

    void Renderer::DrawRect(float x, float y, float width, float height, float thickness, SDL_Color color, float rotation, float zIndex)
    {
        float wpp = GetWorldPerPixel();

        Primitive p;
        p.Mode = PrimitiveMode::Shape;
        p.Center = {x, y};
        p.HalfSize = {std::abs(width) * 0.5f, std::abs(height) * 0.5f};
        p.Rotation = glm::radians(rotation);
        p.Z = zIndex;
        p.Color = ToVec4(color);
        p.Stroke = std::max(thickness, wpp);
        p.Padding = wpp * 2.0f;
        p.Blend = s_BlendMode;
        SubmitPrimitive(p);
    }

    void Renderer::DrawTexture(std::shared_ptr<Texture> texture, float x, float y, float w, float h, SDL_Color tint, float rotation, float zIndex, const SDL_FRect *srcRect)
    {
        SpriteDesc desc;
        desc.TextureRef = std::move(texture);
        desc.Position = {x, y};
        desc.Size = {w, h};
        desc.Tint = tint;
        desc.Rotation = rotation;
        desc.Z = zIndex;
        desc.SourceRect = srcRect;
        DrawSprite(desc);
    }

    void Renderer::DrawSprite(const SpriteDesc &desc)
    {
        if (!desc.TextureRef || !desc.TextureRef->GetID())
            return;

        Primitive p;
        p.Center = desc.Position;
        p.HalfSize = desc.Size * 0.5f;
        p.Rotation = glm::radians(desc.Rotation);
        p.Z = desc.Z;
        p.Color = ToVec4(desc.Tint);
        p.Texture = desc.TextureRef->GetID();
        p.Blend = s_BlendMode;

        float u0 = 0.0f, vTop = 1.0f, u1 = 1.0f, vBottom = 0.0f;
        if (desc.SourceRect)
        {
            float tw = static_cast<float>(desc.TextureRef->GetWidth());
            float th = static_cast<float>(desc.TextureRef->GetHeight());
            if (tw > 0.0f && th > 0.0f)
            {
                u0 = desc.SourceRect->x / tw;
                u1 = (desc.SourceRect->x + desc.SourceRect->w) / tw;
                vTop = 1.0f - desc.SourceRect->y / th;
                vBottom = 1.0f - (desc.SourceRect->y + desc.SourceRect->h) / th;
            }
        }

        if (desc.FlipX)
            std::swap(u0, u1);
        if (desc.FlipY)
            std::swap(vTop, vBottom);

        p.UV = {u0, vTop, u1, vBottom};
        SubmitPrimitive(p);
    }

    void Renderer::DrawLine(float x1, float y1, float x2, float y2, SDL_Color color, float zIndex)
    {
        DrawLine(x1, y1, x2, y2, GetWorldPerPixel(), color, zIndex);
    }

    void Renderer::DrawLine(float x1, float y1, float x2, float y2, float width, SDL_Color color, float zIndex)
    {
        glm::vec2 a(x1, y1), b(x2, y2);
        glm::vec2 d = b - a;
        float len = std::sqrt(d.x * d.x + d.y * d.y);
        float wpp = GetWorldPerPixel();
        float half = std::max(width, wpp) * 0.5f;

        Primitive p;
        p.Mode = PrimitiveMode::Shape;
        p.Center = (a + b) * 0.5f;
        p.HalfSize = {len * 0.5f, half};
        p.Rotation = std::atan2(d.y, d.x);
        p.Z = zIndex;
        p.Color = ToVec4(color);
        p.Padding = wpp;
        p.Blend = s_BlendMode;
        SubmitPrimitive(p);
    }

    void Renderer::DrawCircle(float cx, float cy, float radius,
                              SDL_Color color, float aa, float zIndex)
    {
        Primitive p;
        p.Mode = PrimitiveMode::Shape;
        p.Center = {cx, cy};
        p.HalfSize = {radius, radius};
        p.CornerRadius = radius;
        p.Z = zIndex;
        p.Color = ToVec4(color);
        p.Softness = aa;
        p.Padding = GetWorldPerPixel() * 2.0f;
        p.Blend = s_BlendMode;
        SubmitPrimitive(p);
    }

    void Renderer::DrawLight(const glm::vec2 &center, float radius, float rotationDegrees, const glm::vec4 &color,
                             float falloff, float innerAngleDegrees, float outerAngleDegrees, float zIndex)
    {
        Primitive p;
        p.Mode = PrimitiveMode::Light;
        p.Center = center;
        p.HalfSize = {radius, radius};
        p.Rotation = glm::radians(rotationDegrees);
        p.Z = zIndex;
        p.Color = color;
        p.CornerRadius = falloff;
        p.Stroke = outerAngleDegrees > 0.0f ? glm::radians(outerAngleDegrees) : 0.0f;
        p.Softness = glm::radians(std::max(0.0f, innerAngleDegrees));
        p.Blend = s_BlendMode;
        SubmitPrimitive(p);
    }

    void Renderer::DrawRing(float cx, float cy, float radius, float thickness,
                            SDL_Color color, float aa, float zIndex)
    {
        Primitive p;
        p.Mode = PrimitiveMode::Shape;
        p.Center = {cx, cy};
        p.HalfSize = {radius, radius};
        p.CornerRadius = radius;
        p.Stroke = std::max(thickness, GetWorldPerPixel());
        p.Z = zIndex;
        p.Color = ToVec4(color);
        p.Softness = aa;
        p.Padding = GetWorldPerPixel() * 2.0f;
        p.Blend = s_BlendMode;
        SubmitPrimitive(p);
    }

    void Renderer::DrawRoundedRect(float cx, float cy, float halfW, float halfH,
                                   float cornerRadius, SDL_Color color,
                                   float aa, float zIndex)
    {
        Primitive p;
        p.Mode = PrimitiveMode::Shape;
        p.Center = {cx, cy};
        p.HalfSize = {halfW, halfH};
        p.CornerRadius = cornerRadius;
        p.Z = zIndex;
        p.Color = ToVec4(color);
        p.Softness = aa;
        p.Padding = GetWorldPerPixel() * 2.0f;
        p.Blend = s_BlendMode;
        SubmitPrimitive(p);
    }

    void Renderer::DrawCapsuleLine(float x1, float y1, float x2, float y2,
                                   float width, SDL_Color color,
                                   float aa, float zIndex)
    {
        glm::vec2 a(x1, y1), b(x2, y2);
        glm::vec2 d = b - a;
        float len = std::sqrt(d.x * d.x + d.y * d.y);

        Primitive p;
        p.Mode = PrimitiveMode::Shape;
        p.Center = (a + b) * 0.5f;
        p.HalfSize = {len * 0.5f + width, width};
        p.CornerRadius = width;
        p.Rotation = std::atan2(d.y, d.x);
        p.Z = zIndex;
        p.Color = ToVec4(color);
        p.Softness = aa;
        p.Padding = GetWorldPerPixel() * 2.0f;
        p.Blend = s_BlendMode;
        SubmitPrimitive(p);
    }

    void Renderer::DrawText(const std::string &text, float x, float y, float size, SDL_Color color,
                            TextAlign align, float rotation, float zIndex, const Font *font)
    {
        std::shared_ptr<Font> fallback;
        if (!font)
        {
            fallback = Font::GetDefault();
            font = fallback.get();
        }
        if (!font || text.empty() || size <= 0.0f)
            return;

        float s = size / font->GetBaseSize();
        float rot = glm::radians(rotation);
        float c = std::cos(rot), sn = std::sin(rot);
        glm::vec2 anchor(x, y);
        glm::vec4 col = ToVec4(color);
        for (const char *p = text.data(), *e = text.data() + text.size(); p < e;)
            font->GetGlyph(DecodeUtf8(p, e));
        unsigned int tex = font->GetTextureID();

        float baseline = -font->GetAscent() * s;
        const char *lineStart = text.data();
        const char *end = text.data() + text.size();

        for (const char *lp = lineStart; lp <= end; ++lp)
        {
            if (lp != end && *lp != '\n')
                continue;

            float lineW = font->MeasureLine(lineStart, lp, size);
            float penX = align == TextAlign::Center ? -lineW * 0.5f : align == TextAlign::Right ? -lineW : 0.0f;

            unsigned int prev = 0;
            const char *p = lineStart;
            while (p < lp)
            {
                unsigned int cp = DecodeUtf8(p, lp);
                const Font::Glyph *g = font->GetGlyph(cp);
                if (!g)
                    continue;
                if (prev)
                    penX += font->GetKerning(prev, cp) * s;
                prev = cp;

                if (g->Width > 0.0f)
                {
                    float left = penX + g->OffsetX * s;
                    float top = baseline - g->OffsetY * s;
                    float w = g->Width * s;
                    float h = g->Height * s;
                    glm::vec2 local(left + w * 0.5f, top - h * 0.5f);

                    Primitive prim;
                    prim.Mode = PrimitiveMode::SdfText;
                    prim.Center = anchor + glm::vec2(local.x * c - local.y * sn, local.x * sn + local.y * c);
                    prim.HalfSize = {w * 0.5f, h * 0.5f};
                    prim.Rotation = rot;
                    prim.Z = zIndex;
                    prim.Color = col;
                    prim.Texture = tex;
                    prim.UV = {g->U0, g->V0, g->U1, g->V1};
                    prim.Blend = s_BlendMode;
                    SubmitPrimitive(prim);
                }

                penX += g->Advance * s;
            }

            baseline -= font->GetLineHeight() * s;
            lineStart = lp + 1;
        }
    }

    void Renderer::Present()
    {
    }

    void Renderer::Flush()
    {
        s_RenderingAPI->Flush();
    }

    void Renderer::SetViewMatrix(const glm::mat4 &view)
    {
        s_ViewMatrix = view;
        if (s_RenderingAPI)
            s_RenderingAPI->SetViewMatrix(view);
    }

    void Renderer::SetProjectionMatrix(const glm::mat4 &projection)
    {
        if (s_RenderingAPI)
            s_RenderingAPI->SetProjectionMatrix(projection);
    }

    const glm::mat4 &Renderer::GetViewMatrix()
    {
        return s_ViewMatrix;
    }

    void Renderer::DrawSvg(SvgDocument &doc, float x, float y, float width, float height,
                           SDL_Color tint, float rotation, float zIndex)
    {
        int texW = std::max(1, static_cast<int>(std::ceil(width)));
        int texH = std::max(1, static_cast<int>(std::ceil(height)));

        auto tex = SvgTextureCache::GetOrCreate(doc, texW, texH);
        if (!tex)
            return;

        DrawTexture(tex, x, y, width, height, tint, rotation, zIndex);
    }

    glm::vec2 Renderer::GetViewportSize()
    {
        return s_RenderingAPI->GetViewportSize();
    }

    const RenderStats &Renderer::GetStats()
    {
        return s_RenderingAPI->GetStats();
    }

    void Renderer::ResetStats()
    {
        s_RenderingAPI->ResetStats();
    }
}
