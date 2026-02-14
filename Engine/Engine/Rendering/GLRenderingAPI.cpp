#include <Engine/Rendering/GLRenderingAPI.h>
#include <Engine/Rendering/Shaders/SpriteShader.h>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace SIMPEngine
{
    GLRenderingAPI::GLRenderingAPI()
    {
        m_QuadBatch.reserve(MAX_BATCH_QUADS);
        m_VertexData.resize(MAX_VERTICES);
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
    }

    void GLRenderingAPI::Init()
    {
        glEnable(GL_DEPTH_TEST); // layering
        glDepthFunc(GL_LESS); // blending
        glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);

        m_Shader = std::make_unique<Shader>(
            Shaders::SPRITE_VERT,
            Shaders::SPRITE_FRAG);

        // ------ QUAD BATCHING SETUP ------
        float vertices[] = {
            // pos       //uv
            -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
            0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
            0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f};

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

        // color
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(BatchVertex),
                              (void *)offsetof(BatchVertex, color));
        glEnableVertexAttribArray(4);

        // uv
        glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(BatchVertex),
                              (void *)offsetof(BatchVertex, texCoord));
        glEnableVertexAttribArray(5);

        glBindVertexArray(0);

        // ------ LINE SETUP ------
        glGenVertexArrays(1, &m_LineVAO);
        glGenBuffers(1, &m_LineVBO);

        glBindVertexArray(m_LineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4, nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
            {0, 1}, {1, 1}, {1, 0}, {0, 0}};

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
            v[i].color = c;
            v[i].texCoord = uv[i];
        }
    }

    void GLRenderingAPI::FlushQuads()
    {
        if (m_QuadBatch.empty())
            return;

        for (size_t i = 0; i < m_QuadBatch.size(); i++)
        {
            GenerateQuadVertices(&m_VertexData[i * 4], m_QuadBatch[i]);
        }

        m_Shader->Bind();

        glm::mat4 mvp = m_Projection * m_ViewMatrix;
        m_Shader->SetUniformMat4("uVP", mvp);
        m_Shader->SetUniform1i("uUseTexture", 0);

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_QuadBatch.size() * 4 * sizeof(BatchVertex), m_VertexData.data());

        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, m_QuadBatch.size() * 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        m_Shader->Unbind();

        m_QuadBatch.clear();
        m_IsBatchDirty = false;
    }

    void GLRenderingAPI::DrawQuad(float x, float y, float width, float height, float rotation, SDL_Color color, bool fill, float zIndex)
    {
        if (!fill)
        {
            FlushQuads();

            BatchQuad tempQuad;
            tempQuad.position = glm::vec3(x, y, zIndex);
            tempQuad.size = glm::vec2(width, height);
            tempQuad.rotation = rotation;
            tempQuad.color = color;

            BatchVertex tempVerts[4];
            GenerateQuadVertices(tempVerts, tempQuad);

            m_Shader->Bind();

            glm::mat4 mvp = m_Projection * m_ViewMatrix;
            m_Shader->SetUniformMat4("uVP", mvp);
            m_Shader->SetUniform1i("uUseTexture", 0);

            glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(BatchVertex), tempVerts);

            glBindVertexArray(m_VAO);
            glDrawArrays(GL_LINE_LOOP, 0, 4);
            glBindVertexArray(0);

            m_Shader->Unbind();
            return;
        }

        if (m_QuadBatch.size() >= MAX_BATCH_QUADS)
        {
            FlushQuads();
        }

        BatchQuad quad;
        quad.position = glm::vec3(x, y, zIndex);
        quad.size = glm::vec2(width, height);
        quad.rotation = rotation;
        quad.color = color;
        quad.zIndex = zIndex;

        m_QuadBatch.push_back(quad);
        m_IsBatchDirty = true;
    }

    void GLRenderingAPI::DrawLine(float x1, float y1, float x2, float y2, SDL_Color color)
    {
        FlushQuads();

        float vertices[] = {x1, y1, x2, y2};

        m_Shader->Bind();

        glm::mat4 mvp = m_Projection * m_ViewMatrix;
        m_Shader->SetUniformMat4("uVP", mvp);
        // m_Shader->SetUniform4f("uColor",
        //                        color.r / 255.0f,
        //                        color.g / 255.0f,
        //                        color.b / 255.0f,
        //                        color.a / 255.0f);
        m_Shader->SetUniform1i("uUseTexture", 0);

        glBindVertexArray(m_LineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glDrawArrays(GL_LINES, 0, 2);

        glBindVertexArray(0);

        m_Shader->Unbind();
    }

    void GLRenderingAPI::DrawTexture(std::shared_ptr<Texture> texture, float x, float y, float width, float height, SDL_Color color, float rotation, float zIndex, const SDL_FRect *srcRect)
    {
        FlushQuads();

        if (!texture)
        {
            return;
        }

        BatchQuad tempQuad;
        tempQuad.position = glm::vec3(x, y, zIndex);
        tempQuad.size = glm::vec2(width, height);
        tempQuad.rotation = rotation;
        tempQuad.color = color;

        BatchVertex tempVerts[4];
        GenerateQuadVertices(tempVerts, tempQuad);

        if (srcRect)
        {
            float texWidth = texture->GetWidth();
            float texHeight = texture->GetHeight();

            float u1 = srcRect->x / texWidth;
            float v1 = srcRect->y / texHeight;
            float u2 = (srcRect->x + srcRect->w) / texWidth;
            float v2 = (srcRect->y + srcRect->h) / texHeight;

            // Override UVs
            tempVerts[0].texCoord = glm::vec2(u1, v2); // Top-left
            tempVerts[1].texCoord = glm::vec2(u2, v2); // Top-right
            tempVerts[2].texCoord = glm::vec2(u2, v1); // Bottom-right
            tempVerts[3].texCoord = glm::vec2(u1, v1); // Bottom-left
        }

        m_Shader->Bind();

        glm::mat4 mvp = m_Projection * m_ViewMatrix;
        m_Shader->SetUniformMat4("uVP", mvp);
        m_Shader->SetUniform1i("uUseTexture", 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->GetID());
        m_Shader->SetUniform1i("uTexture", 0);

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(BatchVertex), tempVerts);

        glDepthMask(GL_FALSE);
        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);

        glBindTexture(GL_TEXTURE_2D, 0);
        m_Shader->Unbind();
    }

    void GLRenderingAPI::Flush()
    {
        if (!m_QuadBatch.empty())
        {
            FlushQuads();
        }
    }

    void GLRenderingAPI::ApplyCommonSpriteState(
        float x, float y, float width, float height,
        float rotation, float zIndex,
        SDL_Color color, bool useTexture)
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, zIndex));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(width, height, 1.0f));

        glm::mat4 mvp = m_Projection * m_ViewMatrix * model;

        m_Shader->Bind();
        m_Shader->SetUniformMat4("uVP", mvp);
        m_Shader->SetUniform4f(
            "uColor",
            color.r / 255.0f,
            color.g / 255.0f,
            color.b / 255.0f,
            color.a / 255.0f);
        m_Shader->SetUniform1i("uUseTexture", useTexture ? 1 : 0);
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
        m_IsBatchDirty = false;
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

        glGenRenderbuffers(1, &m_RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RBO);
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
        glClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GLRenderingAPI::Present() {}

    glm::vec2 GLRenderingAPI::TransformPosition(float x, float y) const { return {x, y}; }
    glm::vec2 GLRenderingAPI::TransformSize(float w, float h) const { return {w, h}; }
}