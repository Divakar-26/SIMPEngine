#pragma once
#include <Engine/Scene/Component.h>

#include <random>

namespace SIMPEngine
{
    class Scene;

    class MusicSystem
    {
    public:
        void Update(Scene &scene);

        static void Start(MusicComponent &music, std::mt19937 &random);
        static void Next(MusicComponent &music, std::mt19937 &random);
        static void Stop(MusicComponent &music);

        std::mt19937 &Random() { return m_Random; }

    private:
        std::mt19937 m_Random{std::random_device{}()};
    };
}
