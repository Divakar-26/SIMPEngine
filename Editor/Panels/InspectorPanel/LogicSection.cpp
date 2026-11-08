#include "InspectorPanel.h"

#include <Engine/Scripting/LuaScript.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <system_error>

namespace
{
    const std::vector<std::string> LuaExtensions = {".lua"};

    constexpr const char *ScriptTemplate =
        "local Script = {}\n"
        "\n"
        "function Script:OnCreate()\n"
        "end\n"
        "\n"
        "function Script:OnUpdate(dt)\n"
        "end\n"
        "\n"
        "return Script\n";

    std::string SanitizedName(const std::string &name)
    {
        std::string result;
        for (char c : name)
        {
            const unsigned char u = static_cast<unsigned char>(c);
            if (std::isalnum(u) || c == '_' || c == '-')
                result.push_back(c);
            else if (c == ' ' && !result.empty() && result.back() != '_')
                result.push_back('_');
        }
        return result.empty() ? std::string("Script") : result;
    }

    std::string CreateScriptFile(const std::string &entityName)
    {
        auto directory = SIMPEngine::VFS::Resolve("assets://scripts");
        if (!directory)
            return {};

        std::error_code ec;
        const std::filesystem::path root = EditorUI::Utf8Path(*directory);
        std::filesystem::create_directories(root, ec);
        if (ec)
            return {};

        const std::string base = SanitizedName(entityName);
        std::string filename = base + ".lua";
        for (int i = 2; i < 1000 && std::filesystem::exists(root / EditorUI::Utf8Path(filename), ec); ++i)
            filename = base + "_" + std::to_string(i) + ".lua";

        std::ofstream file(root / EditorUI::Utf8Path(filename), std::ios::binary);
        if (!file)
            return {};
        file << ScriptTemplate;
        file.close();

        return "assets://scripts/" + filename;
    }

    void BindScript(ScriptComponent &script, const std::string &vpath)
    {
        script.Instance.reset();
        script.BindLua(vpath);
    }
}

void InspectorPanel::DrawScriptSection(SIMPEngine::Entity &m_SelectedEntity)
{
    using namespace EditorUI;

    EditorContext &ctx = *m_Context;
    SIMPEngine::Scene *const scene = m_Context->Scene;
    const entt::entity handle = m_SelectedEntity.GetHandle();

    DrawComponentSection<ScriptComponent>(ctx, "Script", m_SelectedEntity, [this, &ctx, scene, handle](ScriptComponent &script)
                                          {
        const bool native = script.InstantiateScript && script.LuaPath.empty();
        if (native)
        {
            PropertyValue("Type", "Native (C++)");
            PropertyValue("State", script.Instance ? "Running" : "Not started", script.Instance ? &Colors::Success : &Colors::TextDim);
            return;
        }

        PropertyValue("Type", "Lua");
        PropertyLabel("File");
        if (Button(script.LuaPath.empty() ? "Choose .lua..." : script.LuaPath.c_str(), ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)) && m_AssetPicker)
        {
            m_AssetPicker->Open("Select Lua Script", "assets://", [this, scene, handle](const std::string &vpath)
                                {
                if (m_Context->Scene != scene)
                    return;
                SIMPEngine::Entity target(handle, scene);
                if (!target || !target.HasComponent<ScriptComponent>())
                    return;
                EditorCommands::RecordEdit(*m_Context);
                BindScript(target.GetComponent<ScriptComponent>(), vpath); }, LuaExtensions);
        }
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                std::string path(static_cast<const char *>(payload->Data));
                if (LowerExtension(path) == ".lua")
                {
                    EditorCommands::RecordEdit(ctx);
                    BindScript(script, path);
                }
            }
            ImGui::EndDragDropTarget();
        }

        PropertyLabel("");
        const float half = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
        if (Button("New Script", ButtonKind::Secondary, ImVec2(half, 0)))
        {
            std::string name = "Script";
            if (auto *tag = scene->GetRegistry().try_get<TagComponent>(handle))
                name = tag->Tag;
            const std::string created = CreateScriptFile(name);
            if (created.empty())
            {
                LOG_ERROR("Could not create a script file under assets://scripts");
            }
            else
            {
                EditorCommands::RecordEdit(ctx);
                BindScript(script, created);
                ctx.OpenScriptRequest = created;
            }
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(script.LuaPath.empty());
        if (Button("Clear", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
        {
            EditorCommands::RecordEdit(ctx);
            script.ClearScript();
        }
        ImGui::EndDisabled();

        if (script.LuaPath.empty())
        {
            PropertyValue("State", "No script", &Colors::TextFaint);
            return;
        }

        const auto *lua = dynamic_cast<const SIMPEngine::LuaScript *>(script.Instance.get());
        if (lua && lua->HasFailed())
            PropertyValue("State", "Failed, see Console", &Colors::Error);
        else
            PropertyValue("State", script.Instance ? "Running" : "Not started", script.Instance ? &Colors::Success : &Colors::TextDim);

        PropertyLabel("");
        if (Button("Edit Script", ButtonKind::Primary, ImVec2(-FLT_MIN, 0)))
            ctx.OpenScriptRequest = script.LuaPath; });
}

void InspectorPanel::DrawLogicSection(SIMPEngine::Entity &m_SelectedEntity)
{
    using namespace EditorUI;

    EditorContext &ctx = *m_Context;

    DrawComponentSection<TimerComponent>(ctx, "Timer", m_SelectedEntity, [](TimerComponent &timer)
                                         {
        PropertyFloat("Interval", &timer.interval, 0.05f, 0.001f, 100000.0f, "%.2f s");
        PropertyBool("Repeat", &timer.repeat);
        PropertyBool("Autostart", &timer.running);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Calls OnTimer in this entity's script every time the interval passes"); });

    DrawComponentSection<TriggerComponent>(ctx, "Trigger Zone", m_SelectedEntity, [](TriggerComponent &trigger)
                                           {
        PropertyBool("Enabled", &trigger.enabled);
        PropertyVec2("Size", &trigger.width, 0.5f, "%.1f", 0.0f, 100000.0f);
        PropertyVec2("Offset", &trigger.offsetX, 0.5f, "%.1f");
        PropertyText("Only Entity", trigger.filter);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Leave empty to react to every entity with a Collision component, or type an entity name");
        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextWrapped("Scripts receive OnTriggerEnter, OnTriggerStay and OnTriggerExit");
        ImGui::PopStyleColor(); });

    DrawComponentSection<PathFollowerComponent>(ctx, "Path Follower", m_SelectedEntity, [&ctx](PathFollowerComponent &path)
                                                {
        PropertyFloat("Speed", &path.speed, 1.0f, 0.0f, 100000.0f, "%.1f");
        PropertyFloat("Wait", &path.waitTime, 0.05f, 0.0f, 1000.0f, "%.2f s");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Pause at every point");

        static const char *modes[] = {"Once", "Loop", "Ping-Pong"};
        int mode = static_cast<int>(path.mode);
        if (PropertySegmented("Mode", &mode, modes, 3))
            path.mode = static_cast<PathMode>(mode);

        PropertyBool("Relative", &path.relative);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Points are offsets from where the entity starts. Off = world positions");
        PropertyBool("Autostart", &path.playing);
        PropertyBool("Face Direction", &path.faceDirection);

        SectionLabel("Points");
        for (size_t i = 0; i < path.points.size(); ++i)
        {
            char label[24];
            std::snprintf(label, sizeof(label), "Point %zu", i + 1);
            PropertyVec2(label, &path.points[i].x, 1.0f, "%.1f");
        }

        PropertyLabel("");
        const float third = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2.0f) / 3.0f;
        if (Button("Add", ButtonKind::Secondary, ImVec2(third, 0)))
        {
            EditorCommands::RecordEdit(ctx);
            const glm::vec2 last = path.points.empty() ? glm::vec2(0.0f, 0.0f) : path.points.back();
            path.points.push_back(last + glm::vec2(64.0f, 0.0f));
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(path.points.empty());
        if (Button("Remove", ButtonKind::Secondary, ImVec2(third, 0)))
        {
            EditorCommands::RecordEdit(ctx);
            path.points.pop_back();
        }
        ImGui::SameLine();
        if (Button("Clear", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
        {
            EditorCommands::RecordEdit(ctx);
            path.points.clear();
        }
        ImGui::EndDisabled(); });

    DrawComponentSection<HealthComponent>(ctx, "Health", m_SelectedEntity, [](HealthComponent &health)
                                          {
        if (PropertyFloat("Max", &health.max, 0.5f, 0.0f, 1000000.0f, "%.1f") && health.current > health.max)
            health.current = health.max;
        PropertyFloat("Current", &health.current, 0.5f, 0.0f, health.max, "%.1f");
        PropertyFloat("Invulnerable", &health.invulnerability, 0.05f, 0.0f, 100.0f, "%.2f s");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Seconds of immunity after taking damage");
        PropertyBool("Destroy On Death", &health.destroyOnDeath);
        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextWrapped("Scripts receive OnDamaged and OnDeath");
        ImGui::PopStyleColor(); });
}
