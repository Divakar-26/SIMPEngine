#pragma once
#include <Engine/Rendering/RenderingAPI.h>
#include <Engine/Rendering/Shader.h>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <memory>

namespace SIMPEngine
{
    struct Vertex2D
    {
        glm::vec3 Position;
        glm::vec2 UV;
        glm::vec4 Color;
        glm::vec2 Local;
        glm::vec4 Shape;
        glm::vec3 Misc;
    };

    class GLRenderingAPI : public RenderingAPI
    {
    public:
        GLRenderingAPI();
        ~GLRenderingAPI() override;

        void Init() override;
        void BeginFrame() override;
        void EndFrame() override;

        void SetClearColor(float r, float g, float b, float a) override;
        void Clear() override;

        void SetViewMatrix(const glm::mat4 &view) override;
        void SetProjectionMatrix(const glm::mat4 &proj) override;
        const glm::mat4 &GetViewMatrix() const override { return m_ViewMatrix; }
        const glm::mat4 &GetProjectionMatrix() const override { return m_Projection; }

        void Submit(const Primitive &primitive) override;
        void Flush() override;

        void BindRenderTarget(Framebuffer *target) override;
        Framebuffer *GetRenderTarget() const override { return m_Target; }

        void ResizeViewport(int width, int height) override;
        glm::vec2 GetViewportSize() override;

        const RenderStats &GetStats() const override { return m_Stats; }
        void ResetStats() override { m_Stats = RenderStats{}; }

    private:
        void CreateWhiteTexture();
        void DrawBatch();
        void WriteQuad(const Primitive &p, float slot);
        void ApplyBlend(BlendMode mode);

        static constexpr size_t MAX_QUADS = 10000;
        static constexpr size_t MAX_VERTICES = MAX_QUADS * 4;
        static constexpr size_t MAX_INDICES = MAX_QUADS * 6;
        static constexpr size_t MAX_PENDING = 200000;
        static constexpr int MAX_TEXTURE_SLOTS = 16;

        float m_ClearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};

        unsigned int m_VAO = 0, m_VBO = 0, m_EBO = 0;
        std::unique_ptr<Shader> m_Shader;
        GLuint m_WhiteTexture = 0;

        std::vector<Primitive> m_Pending;
        std::vector<uint32_t> m_Order;
        std::vector<Vertex2D> m_Vertices;
        size_t m_QuadCount = 0;

        std::array<GLuint, MAX_TEXTURE_SLOTS> m_TextureSlots{};
        int m_UsedTextureSlots = 1;
        BlendMode m_BatchBlend = BlendMode::Alpha;

        glm::mat4 m_Projection = glm::mat4(1.0f);
        glm::mat4 m_ViewMatrix = glm::mat4(1.0f);

        Framebuffer *m_Target = nullptr;
        int m_BackbufferWidth = 1920;
        int m_BackbufferHeight = 1080;

        RenderStats m_Stats;
    };
}
