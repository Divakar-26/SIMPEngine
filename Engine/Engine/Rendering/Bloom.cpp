#include <Engine/Rendering/Bloom.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Rendering/Shader.h>

#include <glad/glad.h>

#include <algorithm>

namespace SIMPEngine
{
    namespace
    {
        constexpr const char *FullscreenVert = R"(
#version 400 core
out vec2 vUV;
void main()
{
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    vUV = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)";

        constexpr const char *DownsampleFrag = R"(
#version 400 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uSource;
uniform vec2 uTexel;
uniform int uPrefilter;
uniform float uThreshold;
uniform float uKnee;

vec3 Prefilter(vec3 c)
{
    float brightness = max(c.r, max(c.g, c.b));
    float soft = clamp(brightness - uThreshold + uKnee, 0.0, 2.0 * uKnee);
    soft = soft * soft / (4.0 * uKnee + 0.00001);
    float contribution = max(soft, brightness - uThreshold) / max(brightness, 0.00001);
    return c * contribution;
}

void main()
{
    vec2 t = uTexel;
    vec3 a = texture(uSource, vUV + t * vec2(-2.0, 2.0)).rgb;
    vec3 b = texture(uSource, vUV + t * vec2(0.0, 2.0)).rgb;
    vec3 c = texture(uSource, vUV + t * vec2(2.0, 2.0)).rgb;
    vec3 d = texture(uSource, vUV + t * vec2(-2.0, 0.0)).rgb;
    vec3 e = texture(uSource, vUV).rgb;
    vec3 f = texture(uSource, vUV + t * vec2(2.0, 0.0)).rgb;
    vec3 g = texture(uSource, vUV + t * vec2(-2.0, -2.0)).rgb;
    vec3 h = texture(uSource, vUV + t * vec2(0.0, -2.0)).rgb;
    vec3 i = texture(uSource, vUV + t * vec2(2.0, -2.0)).rgb;
    vec3 j = texture(uSource, vUV + t * vec2(-1.0, 1.0)).rgb;
    vec3 k = texture(uSource, vUV + t * vec2(1.0, 1.0)).rgb;
    vec3 l = texture(uSource, vUV + t * vec2(-1.0, -1.0)).rgb;
    vec3 m = texture(uSource, vUV + t * vec2(1.0, -1.0)).rgb;

    vec3 color = e * 0.125;
    color += (a + c + g + i) * 0.03125;
    color += (b + d + f + h) * 0.0625;
    color += (j + k + l + m) * 0.125;

    if (uPrefilter == 1)
        color = Prefilter(color);
    FragColor = vec4(max(color, vec3(0.0)), 1.0);
}
)";

        constexpr const char *UpsampleFrag = R"(
#version 400 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uSource;
uniform vec2 uTexel;
uniform float uRadius;
uniform float uIntensity;

void main()
{
    vec2 t = uTexel * uRadius;
    vec3 color = texture(uSource, vUV).rgb * 4.0;
    color += texture(uSource, vUV + vec2(-t.x, 0.0)).rgb * 2.0;
    color += texture(uSource, vUV + vec2(t.x, 0.0)).rgb * 2.0;
    color += texture(uSource, vUV + vec2(0.0, -t.y)).rgb * 2.0;
    color += texture(uSource, vUV + vec2(0.0, t.y)).rgb * 2.0;
    color += texture(uSource, vUV + vec2(-t.x, -t.y)).rgb;
    color += texture(uSource, vUV + vec2(t.x, -t.y)).rgb;
    color += texture(uSource, vUV + vec2(-t.x, t.y)).rgb;
    color += texture(uSource, vUV + vec2(t.x, t.y)).rgb;
    FragColor = vec4(color / 16.0 * uIntensity, 0.0);
}
)";

        constexpr int MaxMips = 6;

        std::unique_ptr<Shader> s_Downsample;
        std::unique_ptr<Shader> s_Upsample;
        GLuint s_VAO = 0;

        void EnsureShared()
        {
            if (!s_Downsample)
                s_Downsample = std::make_unique<Shader>(FullscreenVert, DownsampleFrag);
            if (!s_Upsample)
                s_Upsample = std::make_unique<Shader>(FullscreenVert, UpsampleFrag);
            if (!s_VAO)
                glGenVertexArrays(1, &s_VAO);
        }

        void DrawPass(GLuint fbo, int width, int height, GLuint texture)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glViewport(0, 0, width, height);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }

    void BloomRenderer::ReleaseShared()
    {
        s_Downsample.reset();
        s_Upsample.reset();
        if (s_VAO)
        {
            glDeleteVertexArrays(1, &s_VAO);
            s_VAO = 0;
        }
    }

    BloomRenderer::Chain *BloomRenderer::PrepareChain(int &width, int &height)
    {
        Renderer::Flush();
        RenderingAPI *api = Renderer::GetAPI();
        glm::vec2 viewport = api->GetViewportSize();
        width = std::max(1, static_cast<int>(viewport.x));
        height = std::max(1, static_cast<int>(viewport.y));
        if (width < 8 || height < 8)
            return nullptr;

        EnsureShared();

        Chain &chain = m_Chains[api->GetRenderTarget()];
        if (chain.Width != width || chain.Height != height || chain.Mips.empty())
        {
            chain.Width = width;
            chain.Height = height;
            chain.Mips.clear();
            int w = width, h = height;
            for (int i = 0; i < MaxMips; ++i)
            {
                w = std::max(1, w / 2);
                h = std::max(1, h / 2);
                chain.Mips.push_back(std::make_unique<Framebuffer>(w, h, true));
                if (w <= 8 || h <= 8)
                    break;
            }
        }
        return &chain;
    }

    void BloomRenderer::Apply(const BloomSettings &settings)
    {
        if (settings.Intensity <= 0.0f)
            return;

        int width = 0, height = 0;
        Chain *chain = PrepareChain(width, height);
        if (!chain)
            return;

        Framebuffer *target = Renderer::GetAPI()->GetRenderTarget();
        GLuint source = 0;
        if (target)
        {
            source = target->GetColorAttachment();
        }
        else
        {
            if (!chain->Copy)
                chain->Copy = std::make_unique<Framebuffer>(width, height);
            else
                chain->Copy->Resize(width, height);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, chain->Copy->GetID());
            glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            source = chain->Copy->GetColorAttachment();
        }

        Run(*chain, source, width, height, width, height, true, settings);
    }

    void BloomRenderer::ApplyGlow(unsigned int emissionTexture, int sourceWidth, int sourceHeight, float radius)
    {
        int width = 0, height = 0;
        Chain *chain = PrepareChain(width, height);
        if (!chain)
            return;

        BloomSettings settings;
        settings.Intensity = 1.0f;
        settings.Radius = radius;
        Run(*chain, emissionTexture, sourceWidth, sourceHeight, width, height, false, settings);
    }

    void BloomRenderer::Run(Chain &chain, unsigned int source, int sourceWidth, int sourceHeight, int width, int height, bool prefilter, const BloomSettings &settings)
    {
        Framebuffer *target = Renderer::GetAPI()->GetRenderTarget();
        GLuint targetFbo = target ? target->GetID() : 0;

        glBindVertexArray(s_VAO);
        glDisable(GL_BLEND);

        s_Downsample->Bind();
        glUniform1i(s_Downsample->GetUniformLocation("uSource"), 0);
        glUniform1f(s_Downsample->GetUniformLocation("uThreshold"), settings.Threshold);
        glUniform1f(s_Downsample->GetUniformLocation("uKnee"), std::max(settings.Knee, 0.0001f));
        int srcW = sourceWidth, srcH = sourceHeight;
        GLuint srcTex = source;
        for (size_t i = 0; i < chain.Mips.size(); ++i)
        {
            Framebuffer &mip = *chain.Mips[i];
            glUniform2f(s_Downsample->GetUniformLocation("uTexel"), 1.0f / static_cast<float>(srcW), 1.0f / static_cast<float>(srcH));
            glUniform1i(s_Downsample->GetUniformLocation("uPrefilter"), prefilter && i == 0 ? 1 : 0);
            DrawPass(mip.GetID(), mip.GetWidth(), mip.GetHeight(), srcTex);
            srcW = mip.GetWidth();
            srcH = mip.GetHeight();
            srcTex = mip.GetColorAttachment();
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        s_Upsample->Bind();
        glUniform1i(s_Upsample->GetUniformLocation("uSource"), 0);
        glUniform1f(s_Upsample->GetUniformLocation("uRadius"), std::max(settings.Radius, 0.1f));
        glUniform1f(s_Upsample->GetUniformLocation("uIntensity"), 1.0f);
        for (size_t i = chain.Mips.size() - 1; i > 0; --i)
        {
            Framebuffer &from = *chain.Mips[i];
            Framebuffer &to = *chain.Mips[i - 1];
            glUniform2f(s_Upsample->GetUniformLocation("uTexel"), 1.0f / static_cast<float>(from.GetWidth()), 1.0f / static_cast<float>(from.GetHeight()));
            DrawPass(to.GetID(), to.GetWidth(), to.GetHeight(), from.GetColorAttachment());
        }

        Framebuffer &top = *chain.Mips.front();
        glUniform2f(s_Upsample->GetUniformLocation("uTexel"), 1.0f / static_cast<float>(top.GetWidth()), 1.0f / static_cast<float>(top.GetHeight()));
        glUniform1f(s_Upsample->GetUniformLocation("uIntensity"), settings.Intensity);
        DrawPass(targetFbo, width, height, top.GetColorAttachment());

        s_Upsample->Unbind();
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        if (target)
            Renderer::BeginRenderTarget(*target);
        else
            Renderer::EndRenderTarget();
    }
}
