#include <Engine/Rendering/GLRenderingAPI.h>
#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Rendering/Shaders/Renderer2DShader.h>

#include <Engine/Core/Profiler/EngineProfiler.h>
#include <Engine/Core/Log.h>

#include <Engine/Rendering/GL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>

namespace SIMPEngine
{
#if !defined(NDEBUG) && !defined(SIMP_GLES)
    namespace
    {
        void APIENTRY EngineGLDebugCallback(GLenum source, GLenum type, GLuint id,
                                            GLenum severity, GLsizei length,
                                            const GLchar *message, const void *userParam)
        {
            (void)source;
            (void)type;
            (void)id;
            (void)length;
            (void)userParam;

            if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
                return;

            if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM)
                CORE_ERROR("OpenGL debug message: {}", message ? message : "(no message)");
            else
                CORE_WARN("OpenGL debug message: {}", message ? message : "(no message)");
        }
    }
#endif

    GLRenderingAPI::GLRenderingAPI()
    {
        m_Pending.reserve(4096);
        m_Order.reserve(4096);
        m_Vertices.resize(MAX_VERTICES);
    }

    GLRenderingAPI::~GLRenderingAPI()
    {
        m_Pending.clear();

        if (m_VAO)
            glDeleteVertexArrays(1, &m_VAO);
        if (m_VBO)
            glDeleteBuffers(1, &m_VBO);
        if (m_EBO)
            glDeleteBuffers(1, &m_EBO);
        if (m_WhiteTexture)
            glDeleteTextures(1, &m_WhiteTexture);
    }

    void GLRenderingAPI::Init()
    {
        glViewport(0, 0, m_BackbufferWidth, m_BackbufferHeight);

#if !defined(NDEBUG) && !defined(SIMP_GLES)
        if (glDebugMessageCallback)
        {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(EngineGLDebugCallback, nullptr);
        }
#endif

        m_Shader = std::make_unique<Shader>(Shaders::RENDERER2D_VERT, Shaders::RENDERER2D_FRAG);

        GLint maxUnits = 0;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);
        if (maxUnits < MAX_TEXTURE_SLOTS)
            CORE_ERROR("GPU reports only {} fragment texture units; batching expects {}.", maxUnits, MAX_TEXTURE_SLOTS);

        std::vector<unsigned int> indices(MAX_INDICES);
        for (unsigned int i = 0; i < MAX_QUADS; i++)
        {
            unsigned int v = i * 4;
            unsigned int n = i * 6;
            indices[n + 0] = v + 0;
            indices[n + 1] = v + 1;
            indices[n + 2] = v + 2;
            indices[n + 3] = v + 2;
            indices[n + 4] = v + 3;
            indices[n + 5] = v + 0;
        }

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);

        glBindVertexArray(m_VAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(MAX_VERTICES * sizeof(Vertex2D)), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), reinterpret_cast<const void *>(offsetof(Vertex2D, Position)));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), reinterpret_cast<const void *>(offsetof(Vertex2D, UV)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), reinterpret_cast<const void *>(offsetof(Vertex2D, Color)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), reinterpret_cast<const void *>(offsetof(Vertex2D, Local)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), reinterpret_cast<const void *>(offsetof(Vertex2D, Shape)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), reinterpret_cast<const void *>(offsetof(Vertex2D, Misc)));
        glEnableVertexAttribArray(5);

        glBindVertexArray(0);

        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        CreateWhiteTexture();
        m_TextureSlots[0] = m_WhiteTexture;

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

    void GLRenderingAPI::Submit(const Primitive &primitive)
    {
        if (m_Pending.size() >= MAX_PENDING)
            Flush();

        m_Pending.push_back(primitive);

        switch (primitive.Mode)
        {
        case PrimitiveMode::Sprite:
            m_Stats.quadCount++;
            break;
        case PrimitiveMode::Shape:
            m_Stats.shapeCount++;
            break;
        case PrimitiveMode::SdfText:
            m_Stats.glyphCount++;
            break;
        case PrimitiveMode::Light:
        case PrimitiveMode::Mesh:
            m_Stats.shapeCount++;
            break;
        }
        m_Stats.primitives++;
    }

    void GLRenderingAPI::SubmitMesh(const glm::vec2 *positions, const glm::vec4 *colors, const uint32_t *indices, size_t indexCount, float z, BlendMode blend)
    {
        if (indexCount < 3)
            return;
        if (m_Pending.size() >= MAX_PENDING)
            Flush();

        Primitive p;
        p.Mode = PrimitiveMode::Mesh;
        p.Z = z;
        p.Blend = blend;
        p.MeshFirst = static_cast<uint32_t>(m_MeshPositions.size() / 3);
        p.MeshCount = static_cast<uint32_t>(indexCount / 3);
        for (size_t i = 0; i + 2 < indexCount; i += 3)
        {
            for (size_t k = 0; k < 3; ++k)
            {
                m_MeshPositions.push_back(positions[indices[i + k]]);
                m_MeshColors.push_back(colors[indices[i + k]]);
            }
        }
        Submit(p);
    }

    void GLRenderingAPI::WriteMesh(const Primitive &p)
    {
        const glm::vec4 shape(0.0f);
        const glm::vec3 misc(0.0f, static_cast<float>(PrimitiveMode::Mesh), 1.0f);
        for (uint32_t t = 0; t < p.MeshCount; ++t)
        {
            if (m_QuadCount >= MAX_QUADS)
                DrawBatch();
            const size_t first = static_cast<size_t>(p.MeshFirst + t) * 3;
            Vertex2D *v = &m_Vertices[m_QuadCount * 4];
            for (int i = 0; i < 4; ++i)
            {
                const size_t k = first + static_cast<size_t>(i == 3 ? 0 : i);
                v[i].Position = glm::vec3(m_MeshPositions[k], p.Z);
                v[i].UV = glm::vec2(0.0f);
                v[i].Color = m_MeshColors[k];
                v[i].Local = glm::vec2(0.0f);
                v[i].Shape = shape;
                v[i].Misc = misc;
            }
            m_QuadCount++;
        }
    }

    void GLRenderingAPI::WriteQuad(const Primitive &p, float slot)
    {
        Vertex2D *v = &m_Vertices[m_QuadCount * 4];

        glm::vec2 ext = p.HalfSize + glm::vec2(p.Padding);
        glm::vec2 local[4] = {
            {-ext.x, ext.y},
            {ext.x, ext.y},
            {ext.x, -ext.y},
            {-ext.x, -ext.y}};

        glm::vec2 uv[4] = {
            {p.UV.x, p.UV.y},
            {p.UV.z, p.UV.y},
            {p.UV.z, p.UV.w},
            {p.UV.x, p.UV.w}};

        float c = std::cos(p.Rotation);
        float s = std::sin(p.Rotation);
        glm::vec4 shape(p.HalfSize.x, p.HalfSize.y, p.CornerRadius, p.Stroke);
        glm::vec3 misc(slot, static_cast<float>(p.Mode), p.Softness);

        for (int i = 0; i < 4; i++)
        {
            glm::vec2 r(local[i].x * c - local[i].y * s, local[i].x * s + local[i].y * c);
            v[i].Position = glm::vec3(p.Center + r, p.Z);
            v[i].UV = uv[i];
            v[i].Color = p.Color;
            v[i].Local = local[i];
            v[i].Shape = shape;
            v[i].Misc = misc;
        }

        m_QuadCount++;
    }

    void GLRenderingAPI::ApplyBlend(BlendMode mode)
    {
        switch (mode)
        {
        case BlendMode::Additive:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case BlendMode::Multiply:
            glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
            break;
        default:
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        }
    }

    void GLRenderingAPI::DrawBatch()
    {
        if (m_QuadCount == 0)
            return;

        ApplyBlend(m_BatchBlend);

        for (int i = 0; i < m_UsedTextureSlots; i++)
        {
            glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
            glBindTexture(GL_TEXTURE_2D, m_TextureSlots[static_cast<size_t>(i)]);
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_QuadCount * 4 * sizeof(Vertex2D)), m_Vertices.data(), GL_STREAM_DRAW);

        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_QuadCount * 6), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        m_Stats.drawCalls++;
        m_QuadCount = 0;
        m_UsedTextureSlots = 1;
    }

    void GLRenderingAPI::Flush()
    {
        PROFILE_FUNCTION();

        if (m_Pending.empty())
            return;

        m_Order.resize(m_Pending.size());
        std::iota(m_Order.begin(), m_Order.end(), 0u);
        std::stable_sort(m_Order.begin(), m_Order.end(), [this](uint32_t a, uint32_t b)
                         { return m_Pending[a].Z < m_Pending[b].Z; });

        m_Shader->Bind();
        m_Shader->SetUniformMat4("uVP", m_Projection * m_ViewMatrix);

        m_QuadCount = 0;
        m_UsedTextureSlots = 1;
        m_BatchBlend = m_Pending[m_Order[0]].Blend;

        for (uint32_t index : m_Order)
        {
            const Primitive &p = m_Pending[index];

            if (p.Blend != m_BatchBlend || m_QuadCount >= MAX_QUADS)
            {
                DrawBatch();
                m_BatchBlend = p.Blend;
            }

            if (p.Mode == PrimitiveMode::Mesh)
            {
                WriteMesh(p);
                continue;
            }

            float slot = 0.0f;
            if (p.Texture && p.Texture != m_WhiteTexture)
            {
                int found = -1;
                for (int i = 1; i < m_UsedTextureSlots; i++)
                {
                    if (m_TextureSlots[static_cast<size_t>(i)] == p.Texture)
                    {
                        found = i;
                        break;
                    }
                }

                if (found < 0)
                {
                    if (m_UsedTextureSlots >= MAX_TEXTURE_SLOTS)
                        DrawBatch();
                    found = m_UsedTextureSlots++;
                    m_TextureSlots[static_cast<size_t>(found)] = p.Texture;
                    m_Stats.textureSwitches++;
                }
                slot = static_cast<float>(found);
            }

            WriteQuad(p, slot);
        }

        DrawBatch();
        ApplyBlend(BlendMode::Alpha);
        m_Shader->Unbind();

        m_Pending.clear();
        m_MeshPositions.clear();
        m_MeshColors.clear();
    }

    void GLRenderingAPI::SetViewMatrix(const glm::mat4 &view)
    {
        Flush();
        m_ViewMatrix = view;
    }

    void GLRenderingAPI::SetProjectionMatrix(const glm::mat4 &proj)
    {
        Flush();
        m_Projection = proj;
    }

    void GLRenderingAPI::BeginFrame()
    {
        m_Pending.clear();
        m_MeshPositions.clear();
        m_MeshColors.clear();
        m_QuadCount = 0;
        m_UsedTextureSlots = 1;
    }

    void GLRenderingAPI::EndFrame()
    {
        Flush();
    }

    void GLRenderingAPI::BindRenderTarget(Framebuffer *target)
    {
        Flush();
        m_Target = target;

        if (target)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, target->GetID());
            glViewport(0, 0, target->GetWidth(), target->GetHeight());
        }
        else
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, m_BackbufferWidth, m_BackbufferHeight);
        }
    }

    void GLRenderingAPI::ResizeViewport(int width, int height)
    {
        Flush();
        m_BackbufferWidth = std::max(1, width);
        m_BackbufferHeight = std::max(1, height);
        if (!m_Target)
            glViewport(0, 0, m_BackbufferWidth, m_BackbufferHeight);
    }

    glm::vec2 GLRenderingAPI::GetViewportSize()
    {
        if (m_Target)
            return {static_cast<float>(m_Target->GetWidth()), static_cast<float>(m_Target->GetHeight())};
        return {static_cast<float>(m_BackbufferWidth), static_cast<float>(m_BackbufferHeight)};
    }

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
}
