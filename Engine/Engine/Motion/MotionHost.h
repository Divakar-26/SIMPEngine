#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace SIMPEngine
{
    class Scene;

    class MotionHost
    {
    public:
        MotionHost();
        ~MotionHost();

        MotionHost(const MotionHost &) = delete;
        MotionHost &operator=(const MotionHost &) = delete;

        bool Load(const std::string &scriptPath, bool recording = false);
        void Reload();
        bool PollReload();

        void Step(float dt);
        void Seek(float seconds);
        void Render(int width, int height);

        float Time() const { return m_Time; }
        float Duration() const { return m_Duration; }
        int ReloadCount() const { return m_Reloads; }
        const std::string &Path() const { return m_Path; }
        const std::string &Error() const { return m_Error; }
        Scene *GetScene() const { return m_Scene.get(); }

    private:
        void Build();
        void PushVideoState();
        void ReadVideoState();
        void ResetModules();
        std::filesystem::file_time_type Stamp() const;

        std::unique_ptr<Scene> m_Scene;
        std::string m_Path;
        std::string m_Error;
        float m_Time = 0.0f;
        float m_Duration = 0.0f;
        bool m_Recording = false;
        int m_Reloads = 0;
        std::filesystem::file_time_type m_Stamp{};
    };
}
