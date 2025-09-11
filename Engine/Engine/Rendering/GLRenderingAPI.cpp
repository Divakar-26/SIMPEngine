#include "GLRenderingAPI.h"
#include <glad/glad.h>

namespace SIMPEngine
{
    GLRenderingAPI::GLRenderingAPI(){}

    void GLRenderingAPI::Init()
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glViewport(0, 0, 1920, 1080);
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
        glClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GLRenderingAPI::Present()
    {
    }

    void GLRenderingAPI::DrawQuad(float x, float y, float width, float height, SDL_Color color, bool fill) {

    };
    void GLRenderingAPI::DrawCircle(float x, float y, float r, SDL_Color color) {

    };
    void GLRenderingAPI::DrawLine(float x1, float y1, float x2, float y2, SDL_Color color) {

    };
    void GLRenderingAPI::SetViewMatrix(const glm::mat4 &view) {

    };
    void GLRenderingAPI::DrawTexture(SDL_Texture *texture, float x, float y, float w, float h, SDL_Color tint, float rotation, const SDL_FRect *srcRect ) {

    };
    std::shared_ptr<Texture> GLRenderingAPI::CreateTexture(const char *path) {

    };

    void GLRenderingAPI::Flush() {

    };

    SDL_Renderer * GLRenderingAPI::GetSDLRenderer() {
        return nullptr;
    } 
    void GLRenderingAPI::ResizeViewport(int width, int height) {

    };
};