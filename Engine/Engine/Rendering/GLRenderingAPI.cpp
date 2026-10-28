#include <Engine/Rendering/GLRenderingAPI.h>
#include <Engine/Rendering/Shaders/SpriteShader.h>
#include <Engine/Rendering/Shaders/ShapeShader.h>
#include <Engine/Rendering/Shaders/LineShader.h>

#include <Engine/Core/Profiler/EngineProfiler.h>
#include <Engine/Core/Log.h>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace SIMPEngine
{
    GLRenderingAPI::GLRenderingAPI()
    {
        m_QuadBatch.reserve(MAX_BATCH_QUADS);
        m_VertexData.resize(MAX_VERTICES);
        m_LineVertices.reserve(MAX_LINE_VERTICES);
        m_ShapeInstances.reserve(MAX_SHAPE_INSTANCES);
    }

    GLRenderingAPI::~GLRenderingAPI()
    {
        Flush();

        if (m_VAO)
            glDeleteVertexArrays(1, &m_VAO);
        if (m_VBO)
            glDeleteBuffers(1, &m_VBO);
        if (m_EBO)
            glDeleteBuffers(1, &m_EBO);
        if (m_LineVAO)
            glDeleteVertexArrays(1, &m_LineVAO);
        if (m_LineVBO)
            glDeleteBuffers(1, &m_LineVBO);
        if (m_ShapeVAO)
            glDeleteVertexArrays(1, &m_ShapeVAO);
        if (m_ShapeUnitVBO)
            glDeleteBuffers(1, &m_ShapeUnitVBO);
        if (m_ShapeInstanceVBO)
            glDeleteBuffers(1, &m_ShapeInstanceVBO);
        if (m_WhiteTexture)
            glDeleteTextures(1, &m_WhiteTexture);
        if (m_ColorAttachment)
            glDeleteTextures(1, &m_ColorAttachment);
        if (m_RBO)
            glDeleteRenderbuffers(1, &m_RBO);
        if (m_Framebuffer)
            glDeleteFramebuffers(1, &m_Framebuffer);
    }

    void GLRenderingAPI::Init()
    {
        glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);

        m_Shader = std::make_unique<Shader>(
            Shaders::SPRITE_VERT,
            Shaders::SPRITE_FRAG);

        m_LineShader = std::make_unique<Shader>(
            Shaders::LINE_VERT,
            Shaders::LINE_FRAG);

        m_ShapeShader = std::make_unique<Shader>(
            Shaders::SHAPE_VERT,
            Shaders::SHAPE_FRAG);

        GLint maxUnits = 0;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);
        if (maxUnits < MAX_TEXTURE_SLOTS)
        {
            CORE_ERROR("GPU reports only {} fragment texture units; sprite batching is limited to {} instead of {}.",
                       maxUnits, maxUnits, MAX_TEXTURE_SLOTS);
        }

        // ------ QUAD BATCHING SETUP (colored fills + textured sprites, unified) ------
        std::vector<unsigned int> indices(MAX_INDICES);
        for (unsigned int i = 0; i < MAX_BATCH_QUADS; i++)
        {
            unsigned int offset = i * 4;
            unsigned int indexOffset = i * 6;
            indices[indexOffset + 0] = offset + 0;
            indices[indexOffset + 1] = offset + 1;
            indices[indexOffset + 2] = offset + 2;
            indices[indexOffset + 3] = offset + 2;
            indices[indexOffset + 4] = offset + 3;
            indices[indexOffset + 5] = offset + 0;
        }

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER,
                     MAX_VERTICES * sizeof(BatchVertex),
                     nullptr,
                     GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     indices.size() * sizeof(unsigned int),
                     indices.data(),
                     GL_STATIC_DRAW);

        // local pos
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(BatchVertex),
                              (void *)offsetof(BatchVertex, localPos));
        glEnableVertexAttribArray(0);

        // center
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(BatchVertex),
                              (void *)offsetof(BatchVertex, center));
        glEnableVertexAttribArray(1);

        // size
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(BatchVertex),
                              (void *)offsetof(BatchVertex, size));
        glEnableVertexAttribArray(2);

        // rotation
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(BatchVertex),
                              (void *)offsetof(BatchVertex, rotation));
        glEnableVertexAttribArray(3);

        // uv
        glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(BatchVertex),
                              (void *)offsetof(BatchVertex, texCoord));
        glEnableVertexAttribArray(4);

        // texture slot index
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(BatchVertex),
                              (void *)offsetof(BatchVertex, texIndex));
        glEnableVertexAttribArray(5);

        // color
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(BatchVertex),
                              (void *)offsetof(BatchVertex, color));
        glEnableVertexAttribArray(6);

        glBindVertexArray(0);

        // ------ LINE BATCH SETUP ------
        glGenVertexArrays(1, &m_LineVAO);
        glGenBuffers(1, &m_LineVBO);

        glBindVertexArray(m_LineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
        glBufferData(GL_ARRAY_BUFFER, MAX_LINE_VERTICES * sizeof(LineVertex), nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void *)offsetof(LineVertex, pos));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void *)offsetof(LineVertex, color));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // ------ SHAPE BATCH SETUP (instanced SDF shapes) ------
        float unitQuad[] = {
            -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};

        glGenVertexArrays(1, &m_ShapeVAO);
        glGenBuffers(1, &m_ShapeUnitVBO);
        glGenBuffers(1, &m_ShapeInstanceVBO);

        glBindVertexArray(m_ShapeVAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_ShapeUnitVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(unitQuad), unitQuad, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribDivisor(0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, m_ShapeInstanceVBO);
        glBufferData(GL_ARRAY_BUFFER, MAX_SHAPE_INSTANCES * sizeof(ShapeInstance), nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ShapeInstance), (void *)offsetof(ShapeInstance, boundsCenter));
        glEnableVertexAttribArray(1);
        glVertexAttribDivisor(1, 1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ShapeInstance), (void *)offsetof(ShapeInstance, boundsHalf));
        glEnableVertexAttribArray(2);
        glVertexAttribDivisor(2, 1);

        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(ShapeInstance), (void *)offsetof(ShapeInstance, dataA));
        glEnableVertexAttribArray(3);
        glVertexAttribDivisor(3, 1);

        glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(ShapeInstance), (void *)offsetof(ShapeInstance, dataB));
        glEnableVertexAttribArray(4);
        glVertexAttribDivisor(4, 1);

        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(ShapeInstance), (void *)offsetof(ShapeInstance, radius));
        glEnableVertexAttribArray(5);
        glVertexAttribDivisor(5, 1);

        glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(ShapeInstance), (void *)offsetof(ShapeInstance, shapeType));
        glEnableVertexAttribArray(6);
        glVertexAttribDivisor(6, 1);

        glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(ShapeInstance), (void *)offsetof(ShapeInstance, aa));
        glEnableVertexAttribArray(7);
        glVertexAttribDivisor(7, 1);

        glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(ShapeInstance), (void *)offsetof(ShapeInstance, color));
        glEnableVertexAttribArray(8);
        glVertexAttribDivisor(8, 1);

        glBindVertexArray(0);

        // ------ WHITE FALLBACK TEXTURE + SAMPLER ARRAY ------
        CreateWhiteTexture();
        ResetTextureSlots();

        m_Shader->Bind();
        int samplers[MAX_TEXTURE_SLOTS];
        for (int i = 0; i < MAX_TEXTURE_SLOTS; i++)
            samplers[i] = i;
        glUniform1iv(m_Shader->GetUniformLocation("uTextures"), MAX_TEXTURE_SLOTS, samplers);
        m_Shader->Unbind();
    }

    void GLRenderingAPI::CreateWhiteTexture()
    {
        glGenTextures(1, &m_WhiteTexture);
        glBindTexture(GL_TEXTURE_2D, m_WhiteTexture);
        unsigned char pixel[4] = {255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GLRenderingAPI::ResetTextureSlots()
    {
        m_TextureSlots[0] = m_WhiteTexture;
        m_UsedTextureSlots = 1;
    }

    float GLRenderingAPI::AcquireTextureSlot(GLuint texID)
    {
        if (texID == 0)
            return 0.0f;

        for (int i = 1; i < m_UsedTextureSlots; i++)
        {
            if (m_TextureSlots[i] == texID)
                return (float)i;
        }

        if (m_UsedTextureSlots >= MAX_TEXTURE_SLOTS)
        {
            // Out of slots for this batch: draw what we have so far (if
            // anything is pending) and start a fresh slot table.
            if (!m_QuadBatch.empty())
                FlushQuads();
            else
                ResetTextureSlots();
        }

        m_TextureSlots[m_UsedTextureSlots] = texID;
        m_Stats.textureSwitches++;
        return (float)(m_UsedTextureSlots++);
    }

    void GLRenderingAPI::FlushAllExcept(BatchCategory keepPending)
    {
        if (keepPending != BatchCategory::Quad)
            FlushQuads();
        if (keepPending != BatchCategory::Line)
            FlushLines();
        if (keepPending != BatchCategory::Shape)
            FlushShapes();
    }

    void GLRenderingAPI::GenerateQuadVertices(
        BatchVertex *v,
        const BatchQuad &q)
    {
        glm::vec2 local[4] = {
            {-0.5f, 0.5f},
            {0.5f, 0.5f},
            {0.5f, -0.5f},
            {-0.5f, -0.5f}};

        glm::vec2 uv[4] = {
            {q.uvRect.x, q.uvRect.y},
            {q.uvRect.z, q.uvRect.y},
            {q.uvRect.z, q.uvRect.w},
            {q.uvRect.x, q.uvRect.w}};

        glm::vec4 c(
            q.color.r / 255.f,
            q.color.g / 255.f,
            q.color.b / 255.f,
            q.color.a / 255.f);

        float rotRad = glm::radians(q.rotation);

        for (int i = 0; i < 4; i++)
        {
            v[i].localPos = local[i];
            v[i].center = q.position;
            v[i].size = q.size;
            v[i].rotation = rotRad;
            v[i].texCoord = uv[i];
            v[i].texIndex = q.texIndex;
            v[i].color = c;
        }
    }

    void GLRenderingAPI::SubmitQuad(BatchQuad quad, GLuint textureID)
    {
        FlushAllExcept(BatchCategory::Quad);

        if (m_QuadBatch.size() >= MAX_BATCH_QUADS)
            FlushQuads();

        quad.texIndex = AcquireTextureSlot(textureID);

        m_QuadBatch.push_back(quad);
        m_IsBatchDirty = true;
    }

    void GLRenderingAPI::FlushQuads()
    {
        PROFILE_FUNCTION();

        if (m_QuadBatch.empty())
            return;

        if (m_SortByZIndex)
        {
            PROFILE_SCOPE("FlushQuads/Sort");
            std::stable_sort(m_QuadBatch.begin(), m_QuadBatch.end(),
                              [](const BatchQuad &a, const BatchQuad &b)
                              { return a.zIndex < b.zIndex; });
        }

        {
            PROFILE_SCOPE("FlushQuads/GenerateVertices");
            for (size_t i = 0; i < m_QuadBatch.size(); i++)
            {
                GenerateQuadVertices(&m_VertexData[i * 4], m_QuadBatch[i]);
            }
        }

        m_Shader->Bind();

        glm::mat4 mvp = m_Projection * m_ViewMatrix;
        m_Shader->SetUniformMat4("uVP", mvp);

        {
            PROFILE_SCOPE("FlushQuads/BindTextures");
            for (int i = 0; i < m_UsedTextureSlots; i++)
            {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, m_TextureSlots[i]);
            }
        }

        {
            PROFILE_SCOPE("FlushQuads/Upload");
            glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, m_QuadBatch.size() * 4 * sizeof(BatchVertex), m_VertexData.data());
        }

        {
            PROFILE_SCOPE("FlushQuads/Draw");
            glBindVertexArray(m_VAO);
            glDrawElements(GL_TRIANGLES, (GLsizei)(m_QuadBatch.size() * 6), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        m_Stats.drawCalls++;
        m_Stats.quadCount += (int)m_QuadBatch.size();

        m_QuadBatch.clear();
        m_IsBatchDirty = false;
        ResetTextureSlots();
    }

    void GLRenderingAPI::DrawQuad(float x, float y, float width, float height, float rotation, SDL_Color color, bool fill, float zIndex)
    {
        if (!fill)
        {
            // Wireframe outline: four line segments through the line batch
            // instead of an immediate GL_LINE_LOOP draw.
            float rad = glm::radians(rotation);
            float c = std::cos(rad);
            float s = std::sin(rad);
            glm::vec2 half(width * 0.5f, height * 0.5f);
            glm::vec2 local[4] = {
                {-half.x, half.y}, {half.x, half.y}, {half.x, -half.y}, {-half.x, -half.y}};

            glm::vec2 world[4];
            for (int i = 0; i < 4; i++)
            {
                glm::vec2 r(local[i].x * c - local[i].y * s, local[i].x * s + local[i].y * c);
                world[i] = r + glm::vec2(x, y);
            }

            for (int i = 0; i < 4; i++)
                AddLineToBatch(world[i].x, world[i].y, world[(i + 1) % 4].x, world[(i + 1) % 4].y, color);

            return;
        }

        BatchQuad quad;
        quad.position = glm::vec3(x, y, zIndex);
        quad.size = glm::vec2(width, height);
        quad.rotation = rotation;
        quad.color = color;
        quad.zIndex = zIndex;
        quad.uvRect = glm::vec4(0.0f, 1.0f, 1.0f, 0.0f);

        SubmitQuad(quad, 0);
    }

    void GLRenderingAPI::AddLineToBatch(float x1, float y1, float x2, float y2, SDL_Color color)
    {
        FlushAllExcept(BatchCategory::Line);

        if (m_LineVertices.size() + 2 > MAX_LINE_VERTICES)
            FlushLines();

        glm::vec4 c(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        m_LineVertices.push_back({{x1, y1}, c});
        m_LineVertices.push_back({{x2, y2}, c});
    }

    void GLRenderingAPI::DrawLine(float x1, float y1, float x2, float y2, SDL_Color color)
    {
        AddLineToBatch(x1, y1, x2, y2, color);
    }

    void GLRenderingAPI::FlushLines()
    {
        PROFILE_FUNCTION();

        if (m_LineVertices.empty())
            return;

        m_LineShader->Bind();
        glm::mat4 mvp = m_Projection * m_ViewMatrix;
        m_LineShader->SetUniformMat4("uVP", mvp);

        glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_LineVertices.size() * sizeof(LineVertex), m_LineVertices.data());

        glBindVertexArray(m_LineVAO);
        glDrawArrays(GL_LINES, 0, (GLsizei)m_LineVertices.size());
        glBindVertexArray(0);

        m_LineShader->Unbind();

        m_Stats.drawCalls++;
        m_Stats.lineSegmentCount += (int)(m_LineVertices.size() / 2);

        m_LineVertices.clear();
    }

    void GLRenderingAPI::DrawTexture(std::shared_ptr<Texture> texture, float x, float y, float width, float height, SDL_Color color, float rotation, float zIndex, const SDL_FRect *srcRect)
    {
        if (!texture)
        {
            return;
        }

        BatchQuad quad;
        quad.position = glm::vec3(x, y, zIndex);
        quad.size = glm::vec2(width, height);
        quad.rotation = rotation;
        quad.color = color;
        quad.zIndex = zIndex;

        if (srcRect)
        {
            float texWidth = texture->GetWidth();
            float texHeight = texture->GetHeight();

            float u1 = srcRect->x / texWidth;
            float v1 = srcRect->y / texHeight;
            float u2 = (srcRect->x + srcRect->w) / texWidth;
            float v2 = (srcRect->y + srcRect->h) / texHeight;

            // Textures are uploaded vertically flipped, so image-space Y is inverted here.
            quad.uvRect = glm::vec4(u1, 1.0f - v1, u2, 1.0f - v2);
        }
        else
        {
            quad.uvRect = glm::vec4(0.0f, 1.0f, 1.0f, 0.0f);
        }

        SubmitQuad(quad, texture->GetID());
    }

    void GLRenderingAPI::PushShapeInstance(const ShapeInstance &instance)
    {
        FlushAllExcept(BatchCategory::Shape);

        if (m_ShapeInstances.size() >= MAX_SHAPE_INSTANCES)
            FlushShapes();

        m_ShapeInstances.push_back(instance);
    }

    void GLRenderingAPI::DrawCircle(float cx, float cy, float radius, SDL_Color color, float aa, float zIndex)
    {
        float pad = aa * 2.0f;

        ShapeInstance inst{};
        inst.boundsCenter = {cx, cy};
        inst.boundsHalf = {radius + pad, radius + pad};
        inst.dataA = {cx, cy};
        inst.dataB = {0.0f, 0.0f};
        inst.radius = radius;
        inst.shapeType = 0.0f;
        inst.aa = aa;
        inst.color = {color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f};

        PushShapeInstance(inst);
    }

    void GLRenderingAPI::DrawRoundedRect(float cx, float cy, float halfW, float halfH,
                                         float cornerRadius, SDL_Color color,
                                         float aa, float zIndex)
    {
        float pad = aa * 2.0f;

        ShapeInstance inst{};
        inst.boundsCenter = {cx, cy};
        inst.boundsHalf = {halfW + cornerRadius + pad, halfH + cornerRadius + pad};
        inst.dataA = {cx, cy};
        inst.dataB = {halfW, halfH};
        inst.radius = cornerRadius;
        inst.shapeType = 1.0f;
        inst.aa = aa;
        inst.color = {color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f};

        PushShapeInstance(inst);
    }

    void GLRenderingAPI::DrawCapsuleLine(float x1, float y1, float x2, float y2,
                                         float width, SDL_Color color,
                                         float aa, float zIndex)
    {
        float pad = aa * 2.0f;
        float minX = std::min(x1, x2) - width - pad;
        float minY = std::min(y1, y2) - width - pad;
        float maxX = std::max(x1, x2) + width + pad;
        float maxY = std::max(y1, y2) + width + pad;

        ShapeInstance inst{};
        inst.boundsCenter = {(minX + maxX) * 0.5f, (minY + maxY) * 0.5f};
        inst.boundsHalf = {(maxX - minX) * 0.5f, (maxY - minY) * 0.5f};
        inst.dataA = {x1, y1};
        inst.dataB = {x2, y2};
        inst.radius = width;
        inst.shapeType = 2.0f;
        inst.aa = aa;
        inst.color = {color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f};

        PushShapeInstance(inst);
    }

    void GLRenderingAPI::FlushShapes()
    {
        PROFILE_FUNCTION();

        if (m_ShapeInstances.empty())
            return;

        m_ShapeShader->Bind();
        glm::mat4 mvp = m_Projection * m_ViewMatrix;
        m_ShapeShader->SetUniformMat4("uVP", mvp);

        glBindBuffer(GL_ARRAY_BUFFER, m_ShapeInstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_ShapeInstances.size() * sizeof(ShapeInstance), m_ShapeInstances.data());

        glBindVertexArray(m_ShapeVAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)m_ShapeInstances.size());
        glBindVertexArray(0);

        m_ShapeShader->Unbind();

        m_Stats.drawCalls++;
        m_Stats.shapeCount += (int)m_ShapeInstances.size();

        m_ShapeInstances.clear();
    }

    void GLRenderingAPI::Flush()
    {
        FlushQuads();
        FlushLines();
        FlushShapes();
    }

    void GLRenderingAPI::ApplyCommonSpriteState(
        float x, float y, float width, float height,
        float rotation, float zIndex,
        SDL_Color color, bool useTexture)
    {
        // Legacy helper predating the batch pipeline. Not called internally
        // anymore -- DrawQuad/DrawTexture both go through SubmitQuad now.
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, zIndex));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(width, height, 1.0f));

        glm::mat4 mvp = m_Projection * m_ViewMatrix * model;

        m_Shader->Bind();
        m_Shader->SetUniformMat4("uVP", mvp);
        (void)useTexture; // uUseTexture no longer exists in the unified shader
    }

    void GLRenderingAPI::SetViewMatrix(const glm::mat4 &view)
    {
        m_ViewMatrix = view;
        Flush();
    }

    void GLRenderingAPI::SetProjectionMatrix(const glm::mat4 &proj)
    {
        m_Projection = proj;
        Flush();
    }

    void GLRenderingAPI::BeginFrame()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
        glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);

        m_QuadBatch.clear();
        m_LineVertices.clear();
        m_ShapeInstances.clear();
        m_IsBatchDirty = false;
        ResetTextureSlots();

        m_Stats = RenderStats{};
    }

    void GLRenderingAPI::EndFrame()
    {
        Flush();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLRenderingAPI::InitFramebuffer(int width, int height)
    {
        if (m_Framebuffer)
        {
            glDeleteFramebuffers(1, &m_Framebuffer);
            glDeleteTextures(1, &m_ColorAttachment);
            glDeleteRenderbuffers(1, &m_RBO);
        }
        glGenFramebuffers(1, &m_Framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

        glGenTextures(1, &m_ColorAttachment);
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLRenderingAPI::ResizeViewport(int width, int height)
    {
        Flush();

        glViewport(0, 0, width, height);
        std::cout << "Viewport changed" << std::endl;
        m_ViewportWidth = width;
        m_ViewportHeight = height;
        InitFramebuffer(width, height);
    }

    unsigned int GLRenderingAPI::GetViewportTexture() { return m_ColorAttachment; }

    void GLRenderingAPI::SetClearColor(float r, float g, float b, float a)
    {
        m_ClearColor[0] = r;
        m_ClearColor[1] = g;
        m_ClearColor[2] = b;
        m_ClearColor[3] = a;
    }

    void GLRenderingAPI::Clear()
    {
        PROFILE_SCOPE("GL/Clear");
        glClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void GLRenderingAPI::Present() {}

    glm::vec2 GLRenderingAPI::TransformPosition(float x, float y) const { return {x, y}; }
    glm::vec2 GLRenderingAPI::TransformSize(float w, float h) const { return {w, h}; }
}