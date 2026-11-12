#include <Engine/Motion/MotionHost.h>

#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scripting/LuaInclude.h>
#include <Engine/Scripting/LuaRuntime.h>

#include <algorithm>

namespace SIMPEngine
{
    namespace
    {
        constexpr const char *BaselineScript = R"LUA(
if __motion_baseline == nil then
    __motion_baseline = {}
    for key in pairs(package.loaded) do
        __motion_baseline[key] = true
    end
else
    for key in pairs(package.loaded) do
        if not __motion_baseline[key] then
            package.loaded[key] = nil
        end
    end
end
)LUA";
    }

    MotionHost::MotionHost() = default;
    MotionHost::~MotionHost() = default;

    bool MotionHost::Load(const std::string &scriptPath, bool recording)
    {
        m_Path = scriptPath;
        m_Recording = recording;
        m_Time = 0.0f;
        m_Reloads = 0;
        Build();
        return m_Error.empty();
    }

    void MotionHost::Reload()
    {
        ++m_Reloads;
        Build();
    }

    std::filesystem::file_time_type MotionHost::Stamp() const
    {
        namespace fs = std::filesystem;
        fs::file_time_type newest{};
        std::string real = m_Path;
        if (auto resolved = VFS::Resolve(m_Path))
            real = *resolved;

        std::error_code ec;
        const fs::path script(real);
        newest = fs::last_write_time(script, ec);
        const fs::path directory = script.parent_path();
        if (directory.empty() || !fs::is_directory(directory, ec))
            return newest;

        int visited = 0;
        for (fs::recursive_directory_iterator it(directory, fs::directory_options::skip_permission_denied, ec), end; !ec && it != end && visited < 800; it.increment(ec))
        {
            ++visited;
            if (it.depth() > 3)
            {
                it.disable_recursion_pending();
                continue;
            }
            if (it->path().extension() != ".lua")
                continue;
            std::error_code stampError;
            const auto time = fs::last_write_time(it->path(), stampError);
            if (!stampError && time > newest)
                newest = time;
        }
        return newest;
    }

    bool MotionHost::PollReload()
    {
        const auto stamp = Stamp();
        if (stamp == m_Stamp)
            return false;
        Reload();
        return true;
    }

    void MotionHost::ResetModules()
    {
        LuaRuntime::Get().ExecuteString(BaselineScript, "motion modules");
    }

    void MotionHost::PushVideoState()
    {
        lua_State *L = LuaRuntime::Get().GetState();
        if (!L)
            return;
        lua_getglobal(L, "Video");
        if (lua_istable(L, -1))
        {
            lua_pushnumber(L, m_Time);
            lua_setfield(L, -2, "Time");
            lua_pushboolean(L, 1);
            lua_setfield(L, -2, "Driven");
            lua_pushboolean(L, m_Recording ? 1 : 0);
            lua_setfield(L, -2, "Recording");
        }
        lua_pop(L, 1);
    }

    void MotionHost::ReadVideoState()
    {
        lua_State *L = LuaRuntime::Get().GetState();
        if (!L)
            return;
        lua_getglobal(L, "Video");
        if (lua_istable(L, -1))
        {
            lua_getfield(L, -1, "Duration");
            if (lua_isnumber(L, -1))
                m_Duration = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }

    void MotionHost::Build()
    {
        LuaRuntime &lua = LuaRuntime::Get();
        lua.Initialize();
        ResetModules();
        lua.ClearScriptCache();
        lua.ClearLastError();
        m_Error.clear();
        m_Duration = 0.0f;
        m_Stamp = Stamp();

        lua.ExecuteString("Video.Duration = nil", "motion reset");
        PushVideoState();

        m_Scene = std::make_unique<Scene>("Motion");
        Entity camera = m_Scene->CreateEntity("Camera");
        auto &cc = camera.AddComponent<CameraComponent>(1.0f, glm::vec2(0.0f));
        cc.primary = true;
        cc.Camera.SetManualControl(false);
        m_Scene->CreateEntity("Video").AddComponent<ScriptComponent>().BindLua(m_Path);

        try
        {
            m_Scene->OnUpdate(0.0f);
        }
        catch (const std::exception &e)
        {
            m_Error = e.what();
        }
        ReadVideoState();
        if (m_Error.empty())
            m_Error = lua.GetLastError();
        if (m_Error.empty())
            CORE_INFO("Motion: {} {} ({:.1f} s)", m_Reloads > 0 ? "reloaded" : "loaded", m_Path, m_Duration);
        else
            CORE_ERROR("Motion: {} failed", m_Path);
    }

    void MotionHost::Step(float dt)
    {
        if (!m_Scene)
            return;
        m_Time += dt;
        PushVideoState();
        try
        {
            m_Scene->OnUpdate(dt);
        }
        catch (const std::exception &e)
        {
            m_Error = e.what();
        }
        ReadVideoState();
        const std::string &luaError = LuaRuntime::Get().GetLastError();
        if (!luaError.empty())
            m_Error = luaError;
    }

    void MotionHost::Seek(float seconds)
    {
        if (!m_Scene)
            return;
        m_Time = std::max(seconds, 0.0f);
        PushVideoState();
        try
        {
            m_Scene->OnUpdate(0.0f);
        }
        catch (const std::exception &e)
        {
            m_Error = e.what();
        }
    }

    void MotionHost::Render(int width, int height)
    {
        if (!m_Scene)
            return;
        auto &registry = m_Scene->GetRegistry();
        for (auto entity : registry.view<CameraComponent>())
            registry.get<CameraComponent>(entity).Camera.SetViewportSize(static_cast<float>(width), static_cast<float>(height));
        Renderer::SetClearColor(0.055f, 0.067f, 0.118f, 1.0f);
        Renderer::Clear();
        try
        {
            m_Scene->OnRender();
        }
        catch (const std::exception &e)
        {
            m_Error = e.what();
        }
        Renderer::EndScene();
    }
}
