#pragma once
#include <Engine/Rendering/RenderingAPI.h>
#include <Engine/Rendering/Shader.h>
#include <Engine/Rendering/Texture.h>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <memory>

namespace SIMPEngine
{
    struct BatchQuad
    {
        glm::vec3 position;
        glm::vec2 size;
        float rotation;
        SDL_Color color;
        float zIndex;
        glm::vec4 uvRect = {0.0f, 1.0f, 1.0f, 0.0f};
        float texIndex = 0.0f;
    };

    struct BatchVertex
    {
        glm::vec2 localPos;
        glm::vec3 center;
        glm::vec2 size;
        float rotation;
        glm::vec2 texCoord;
        float texIndex;
        glm::vec4 color;
    };

    struct LineVertex
    {
        glm::vec2 pos;
        glm::vec4 color;
    };

    struct ShapeInstance
    {
        glm::vec2 boundsCenter;
        glm::vec2 boundsHalf;
        glm::vec2 dataA;
        glm::vec2 dataB;
        float radius;
        float shapeType;
        float aa;
        float _pad = 0.0f;
        glm::vec4 color;
    };

    class GLRenderingAPI : public RenderingAPI
    {
    public:
        GLRenderingAPI();
        virtual ~GLRenderingAPI();

        void Init() override;
        void SetClearColor(float r, float g, float b, float a) override;
        void Clear() override;
        void Present() override;

        void DrawQuad(float x, float y, float width, float height, float rotation = 0.0f, SDL_Color color = {255, 255, 255, 255}, bool fill = true, float zIndex = 0.0f) override;
        void DrawLine(float, float, float, float, SDL_Color) override;
        void DrawTexture(std::shared_ptr<Texture> texture, float x, float y, float width, float height, SDL_Color color, float rotation, float zIndex, const SDL_FRect *srcRect = nullptr) override;
        void DrawCircle(float cx, float cy, float radius,
                        SDL_Color color, float aa = 1.0f, float zIndex = 0.0f) override;

        void DrawRoundedRect(float cx, float cy, float halfW, float halfH,
                             float cornerRadius, SDL_Color color,
                             float aa = 1.0f, float zIndex = 0.0f) override;

        void DrawCapsuleLine(float x1, float y1, float x2, float y2,
                             float width, SDL_Color color,
                             float aa = 1.0f, float zIndex = 0.0f) override;

        std::shared_ptr<Texture> CreateTexture(const char *) override { return nullptr; }

        void Flush() override;
        void ResizeViewport(int, int) override;
        void SetViewMatrix(const glm::mat4 &view) override;
        void SetProjectionMatrix(const glm::mat4 &proj) override;

        unsigned int GetViewportTexture() override;
        glm::vec2 GetViewportSize() { return {m_ViewportWidth, m_ViewportHeight}; }

        void BeginFrame() override;
        void EndFrame() override;
        void InitFramebuffer(int width, int height);

        const RenderStats &GetStats() const override { return m_Stats; }

        void ApplyCommonSpriteState(
            float x, float y, float width, float height,
            float rotation, float zIndex,
            SDL_Color color, bool useTexture);

        int m_ViewportWidth = 1920;
        int m_ViewportHeight = 1080;

    private:
        enum class BatchCategory { Quad, Line, Shape };

        void FlushQuads();
        void GenerateQuadVertices(BatchVertex *vertices, const BatchQuad &quad);
        void SubmitQuad(BatchQuad quad, GLuint textureID);
        float AcquireTextureSlot(GLuint texID);
        void ResetTextureSlots();
        void CreateWhiteTexture();

        void FlushLines();
        void AddLineToBatch(float x1, float y1, float x2, float y2, SDL_Color color);

        void FlushShapes();
        void PushShapeInstance(const ShapeInstance &instance);

        void FlushAllExcept(BatchCategory keepPending);

        float m_ClearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};

        unsigned int m_VAO = 0, m_VBO = 0, m_EBO = 0;
        std::unique_ptr<Shader> m_Shader;

        static constexpr size_t MAX_BATCH_QUADS = 1000;
        static constexpr size_t MAX_VERTICES = MAX_BATCH_QUADS * 4;
        static constexpr size_t MAX_INDICES = MAX_BATCH_QUADS * 6;
        static constexpr int MAX_TEXTURE_SLOTS = 16;

        std::vector<BatchQuad> m_QuadBatch;
        std::vector<BatchVertex> m_VertexData;
        bool m_IsBatchDirty = false;
        bool m_SortByZIndex = true;

        GLuint m_WhiteTexture = 0;
        std::array<GLuint, MAX_TEXTURE_SLOTS> m_TextureSlots{};
        int m_UsedTextureSlots = 1;

        unsigned int m_LineVAO = 0, m_LineVBO = 0;
        std::unique_ptr<Shader> m_LineShader;
        static constexpr size_t MAX_LINE_VERTICES = 4096;
        std::vector<LineVertex> m_LineVertices;

        std::unique_ptr<Shader> m_ShapeShader;
        GLuint m_ShapeVAO = 0;
        GLuint m_ShapeUnitVBO = 0;
        GLuint m_ShapeInstanceVBO = 0;
        static constexpr size_t MAX_SHAPE_INSTANCES = 2000;
        std::vector<ShapeInstance> m_ShapeInstances;

        glm::mat4 m_Projection = glm::mat4(1.0f);
        glm::mat4 m_ViewMatrix = glm::mat4(1.0f);

        glm::vec2 TransformPosition(float x, float y) const;
        glm::vec2 TransformSize(float w, float h) const;

        unsigned int m_Framebuffer = 0;
        unsigned int m_ColorAttachment = 0;
        unsigned int m_RBO = 0;

        RenderStats m_Stats;
    };
}
