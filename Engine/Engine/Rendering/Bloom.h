#pragma once

#include <Engine/Rendering/Framebuffer.h>

#include <memory>
#include <unordered_map>
#include <vector>

namespace SIMPEngine
{
    struct BloomSettings
    {
        bool Enabled = false;
        float Threshold = 0.75f;
        float Knee = 0.4f;
        float Intensity = 0.8f;
        float Radius = 1.0f;
    };

    class BloomRenderer
    {
    public:
        void Apply(const BloomSettings &settings);
        void ApplyGlow(unsigned int emissionTexture, int sourceWidth, int sourceHeight, float radius);
        static void ReleaseShared();

    private:
        struct Chain
        {
            std::unique_ptr<Framebuffer> Copy;
            std::vector<std::unique_ptr<Framebuffer>> Mips;
            int Width = 0;
            int Height = 0;
        };

        Chain *PrepareChain(int &width, int &height);
        void Run(Chain &chain, unsigned int source, int sourceWidth, int sourceHeight, int width, int height, bool prefilter, const BloomSettings &settings);

        std::unordered_map<const void *, Chain> m_Chains;
    };
}
