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

namespace
{
    void SoundField(EditorContext &ctx, const char *label, std::string &sound)
    {
        using namespace EditorUI;
        PropertyText(label, sound);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Drag a .wav or .ogg here from the FileSystem panel. Empty = silent");
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                std::string path(static_cast<const char *>(payload->Data));
                const std::string ext = LowerExtension(path);
                if (ext == ".wav" || ext == ".ogg")
                {
                    EditorCommands::RecordEdit(ctx);
                    sound = path;
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    void NeedsTrigger(SIMPEngine::Entity &entity, const char *message)
    {
        using namespace EditorUI;
        if (entity.HasComponent<TriggerComponent>())
            return;
        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::Warning);
        ImGui::TextWrapped("%s", message);
        ImGui::PopStyleColor();
    }
}

void InspectorPanel::DrawLogicSection(SIMPEngine::Entity &m_SelectedEntity)
{
    using namespace EditorUI;

    EditorContext &ctx = *m_Context;

    DrawComponentSection<ButtonComponent>(ctx, "Button", m_SelectedEntity, [&ctx, &m_SelectedEntity](ButtonComponent &button)
                                          {
        PropertyBool("Enabled", &button.enabled);
        static const char *buttons[] = {"Left", "Right", "Middle"};
        PropertySegmented("Mouse Button", &button.button, buttons, 3);
        PropertyFloat("Hover Scale", &button.hoverScale, 0.005f, 0.1f, 4.0f, "%.2f");
        PropertyFloat("Press Scale", &button.pressScale, 0.005f, 0.1f, 4.0f, "%.2f");
        SoundField(ctx, "Hover Sound", button.hoverSound);
        SoundField(ctx, "Click Sound", button.clickSound);
        PropertyFloat("Volume", &button.volume, 0.01f, 0.0f, 4.0f, "%.2f");
        PropertyFloat("Hit Padding", &button.padding, 0.5f, 0.0f, 1000.0f, "%.1f");
        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextWrapped(m_SelectedEntity.HasComponent<SpriteComponent>() || m_SelectedEntity.HasComponent<RenderComponent>()
                               ? "Scripts receive OnClick, OnHoverEnter and OnHoverExit. Buttons live in the world, so a button on a moving camera should be a child of the Camera entity"
                               : "Add a Sprite or Shape so there is something to click (a Collision box also works)");
        ImGui::PopStyleColor(); });

    DrawComponentSection<TeleporterComponent>(ctx, "Teleporter", m_SelectedEntity, [&ctx, &m_SelectedEntity](TeleporterComponent &portal)
                                              {
        PropertyBool("Enabled", &portal.enabled);
        PropertyText("Destination", portal.target);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Name of the entity to send things to. Make a second Teleporter there to go both ways");
        PropertyVec2("Arrive Offset", &portal.offset.x, 0.5f, "%.1f");
        PropertyBool("Keep Velocity", &portal.keepVelocity);
        SoundField(ctx, "Sound", portal.sound);
        NeedsTrigger(m_SelectedEntity, "Needs a Trigger Zone on this entity. Set its Only Entity to the player so props are not teleported");
        if (!portal.target.empty() && !ctx.Scene->GetEntityByName(portal.target))
        {
            PropertyLabel("");
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::Warning);
            ImGui::TextWrapped("No entity is named \"%s\"", portal.target.c_str());
            ImGui::PopStyleColor();
        }
        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextWrapped("Whatever arrives is not sent back until it has stepped out of the zone");
        ImGui::PopStyleColor(); });

    DrawComponentSection<CameraZoneComponent>(ctx, "Camera Zone", m_SelectedEntity, [&m_SelectedEntity](CameraZoneComponent &zone)
                                              {
        PropertyBool("Enabled", &zone.enabled);
        PropertyFloat("Zoom", &zone.zoom, 0.01f, 0.05f, 20.0f, "%.2f");
        PropertyFloat("Transition", &zone.speed, 0.1f, 0.0f, 100.0f, "%.1f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("How fast the camera eases to the new zoom. 0 snaps");
        PropertyBool("Limit Camera", &zone.setBounds);
        if (zone.setBounds)
        {
            PropertyVec2("Bounds Min", &zone.boundsMin.x, 1.0f, "%.0f");
            PropertyVec2("Bounds Max", &zone.boundsMax.x, 1.0f, "%.0f");
        }
        PropertyBool("Restore On Exit", &zone.restoreOnExit);
        NeedsTrigger(m_SelectedEntity, "Needs a Trigger Zone on this entity. Set its Only Entity to the player");
        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextWrapped("While the player is inside, the main camera zooms and is limited to the bounds");
        ImGui::PopStyleColor(); });

    DrawComponentSection<WorldBoundsComponent>(ctx, "World Bounds", m_SelectedEntity, [](WorldBoundsComponent &bounds)
                                               {
        PropertyBool("Enabled", &bounds.enabled);
        static const char *modes[] = {"Clamp", "Wrap", "Bounce", "Destroy"};
        int mode = static_cast<int>(bounds.mode);
        if (PropertySegmented("Mode", &mode, modes, 4))
            bounds.mode = static_cast<BoundsMode>(mode);
        PropertyVec2("Min", &bounds.min.x, 1.0f, "%.0f");
        PropertyVec2("Max", &bounds.max.x, 1.0f, "%.0f");
        PropertyFloat("Margin", &bounds.margin, 0.5f, 0.0f, 100000.0f, "%.1f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Half the size of the entity, so its edge (not its center) meets the boundary");
        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextWrapped("Clamp: stops at the edge. Wrap: reappears on the opposite side. Bounce: reflects Velocity, Rigid Body or Character Controller motion. Destroy: removes it once outside");
        ImGui::PopStyleColor(); });

    DrawComponentSection<PickableComponent>(ctx, "Pickable", m_SelectedEntity, [&ctx, &m_SelectedEntity](PickableComponent &pickable)
                                            {
        PropertyBool("Enabled", &pickable.enabled);
        static const char *buttons[] = {"Left", "Right", "Middle"};
        PropertySegmented("Mouse Button", &pickable.button, buttons, 3);
        PropertyBool("Keep Grab Offset", &pickable.keepOffset);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("On: the entity stays where you grabbed it. Off: its center jumps to the cursor");
        PropertyFloat("Smoothing", &pickable.smoothing, 0.1f, 0.0f, 100.0f, "%.1f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("0 sticks to the cursor. Higher values trail behind it");
        PropertyFloat("Throw", &pickable.throwStrength, 0.01f, 0.0f, 10.0f, "%.2f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Release while moving to fling it. 0 just drops it. Works with Rigid Body, Character Controller and Velocity");
        PropertyFloat("Lift Scale", &pickable.liftScale, 0.005f, 0.1f, 4.0f, "%.2f");
        PropertyBool("Bring To Front", &pickable.bringToFront);
        PropertyFloat("Grab Padding", &pickable.padding, 0.5f, 0.0f, 1000.0f, "%.1f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Extra pixels around the entity that still count as grabbing it");
        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextWrapped(m_SelectedEntity.HasComponent<SpriteComponent>() || m_SelectedEntity.HasComponent<RenderComponent>()
                               ? "Press and hold on it in the running game. Scripts get OnGrab and OnDrop"
                               : "Add a Sprite or Shape so there is something to grab (a Collision box also works)");
        ImGui::PopStyleColor();
        (void)ctx; });

    DrawComponentSection<FollowComponent>(ctx, "Follow", m_SelectedEntity, [&ctx](FollowComponent &follow)
                                          {
        PropertyBool("Enabled", &follow.enabled);
        PropertyText("Target", follow.target);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Name of the entity to chase (companions, enemies, floating UI)");
        PropertyVec2("Offset", &follow.offset.x, 0.5f, "%.1f");
        PropertyFloat("Smoothing", &follow.smoothing, 0.1f, 0.0f, 100.0f, "%.1f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Higher catches up faster. 0 snaps to the target");
        PropertyFloat("Stop Distance", &follow.stopDistance, 0.5f, 0.0f, 100000.0f, "%.1f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Stay this far away instead of sitting on top of the target");
        PropertyBool2("Axes", "X", &follow.followX, "Y", &follow.followY);
        if (!follow.target.empty() && !ctx.Scene->GetEntityByName(follow.target))
        {
            PropertyLabel("");
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::Warning);
            ImGui::TextWrapped("No entity is named \"%s\"", follow.target.c_str());
            ImGui::PopStyleColor();
        } });

    DrawComponentSection<PickupComponent>(ctx, "Pickup", m_SelectedEntity, [&ctx, &m_SelectedEntity](PickupComponent &pickup)
                                          {
        PropertyBool("Enabled", &pickup.enabled);
        PropertyText("Variable", pickup.key);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Game variable that grows when collected. Show it with {name} in a Text component, read it with Game.Get(name)");
        PropertyFloat("Amount", &pickup.amount, 0.1f, -1000000.0f, 1000000.0f, "%.1f");
        SoundField(ctx, "Sound", pickup.sound);
        PropertyFloat("Volume", &pickup.volume, 0.01f, 0.0f, 4.0f, "%.2f");
        PropertyBool("Destroy On Pickup", &pickup.destroyOnPickup);
        NeedsTrigger(m_SelectedEntity, "Needs a Trigger Zone on this entity. Set its Only Entity to the player so enemies can't collect it"); });

    DrawComponentSection<CheckpointComponent>(ctx, "Checkpoint", m_SelectedEntity, [&ctx, &m_SelectedEntity](CheckpointComponent &checkpoint)
                                              {
        PropertyBool("Enabled", &checkpoint.enabled);
        PropertyVec2("Respawn Offset", &checkpoint.offset.x, 0.5f, "%.1f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Where an entity with a Respawn component reappears, relative to this checkpoint");
        SoundField(ctx, "Sound", checkpoint.sound);
        NeedsTrigger(m_SelectedEntity, "Needs a Trigger Zone on this entity"); });

    DrawComponentSection<RespawnComponent>(ctx, "Respawn", m_SelectedEntity, [&ctx, &m_SelectedEntity](RespawnComponent &respawn)
                                           {
        PropertyFloat("Delay", &respawn.delay, 0.05f, 0.0f, 100.0f, "%.2f s");
        PropertyBool("Restore Health", &respawn.restoreHealth);
        PropertyBool("Fall Limit", &respawn.useKillY);
        if (respawn.useKillY)
        {
            PropertyFloat("Fall Below Y", &respawn.killY, 1.0f, -1000000.0f, 1000000.0f, "%.0f");
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("World Y grows upward, so use a negative number under the lowest platform");
        }
        if (!ctx.IsEditing())
        {
            char state[64];
            std::snprintf(state, sizeof(state), "spawn at %.0f, %.0f", respawn.point.x, respawn.point.y);
            PropertyValue("State", state);
        }
        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextWrapped(m_SelectedEntity.HasComponent<HealthComponent>()
                               ? "Comes back where it started, or at the last Checkpoint it touched. Don't turn on Destroy On Death in Health"
                               : "Comes back after falling. Add Health to also respawn after dying");
        ImGui::PopStyleColor(); });

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
            char label[32];
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

    DrawComponentSection<CharacterControllerComponent>(ctx, "Character Controller", m_SelectedEntity, [&ctx, &m_SelectedEntity](CharacterControllerComponent &cc)
                                                       {
        static const char *modes[] = {"Platformer", "Top Down"};
        int mode = static_cast<int>(cc.mode);
        if (PropertySegmented("Mode", &mode, modes, 2))
        {
            cc.mode = static_cast<ControllerMode>(mode);
            cc.velocity = {0.0f, 0.0f};
        }
        const bool platformer = cc.mode == ControllerMode::Platformer;
        PropertyBool("Player Input", &cc.playerInput);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip(platformer ? "A/D or arrows to move, Space/W/Up to jump. Off = drive it from a script with SetInput and Jump"
                                         : "WASD or arrows. Off = drive it from a script with SetInput");

        SectionLabel("Body");
        PropertyVec2("Size", &cc.size.x, 0.5f, "%.1f", 1.0f, 100000.0f);
        PropertyVec2("Offset", &cc.offset.x, 0.5f, "%.1f");
        PropertyLabel("");
        if (Button("Fit To Sprite", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
        {
            glm::vec2 size{0.0f};
            if (m_SelectedEntity.HasComponent<SpriteComponent>())
            {
                const auto &sprite = m_SelectedEntity.GetComponent<SpriteComponent>();
                size = {sprite.width, sprite.height};
            }
            else if (m_SelectedEntity.HasComponent<RenderComponent>())
            {
                const auto &render = m_SelectedEntity.GetComponent<RenderComponent>();
                size = {render.width, render.height};
            }
            if (size.x > 0.0f && size.y > 0.0f)
            {
                EditorCommands::RecordEdit(ctx);
                const glm::vec2 scale = glm::abs(m_SelectedEntity.GetComponent<TransformComponent>().scale);
                cc.size = size * scale;
                cc.offset = {0.0f, 0.0f};
            }
        }

        SectionLabel("Movement");
        PropertyFloat("Speed", &cc.moveSpeed, 1.0f, 0.0f, 100000.0f, "%.0f");
        PropertyFloat("Acceleration", &cc.acceleration, 10.0f, 0.0f, 1000000.0f, "%.0f");
        PropertyFloat("Deceleration", &cc.deceleration, 10.0f, 0.0f, 1000000.0f, "%.0f");
        if (platformer)
        {
            PropertyFloat("Air Control", &cc.airControl, 0.01f, 0.0f, 1.0f, "%.2f");
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("How much of the acceleration applies in the air");

            SectionLabel("Jump");
            PropertyFloat("Jump Speed", &cc.jumpSpeed, 1.0f, 0.0f, 100000.0f, "%.0f");
            PropertyFloat("Gravity", &cc.gravity, 5.0f, 0.0f, 1000000.0f, "%.0f");
            PropertyFloat("Max Fall", &cc.maxFallSpeed, 5.0f, 0.0f, 1000000.0f, "%.0f");
            PropertyInt("Jumps", &cc.maxJumps, 0.05f, 1, 10);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("2 = double jump");
            PropertyFloat("Jump Cut", &cc.jumpCut, 0.01f, 0.0f, 1.0f, "%.2f");
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("Upward speed kept when the jump key is released early. 1 = fixed height jumps");
            PropertyFloat("Coyote Time", &cc.coyoteTime, 0.005f, 0.0f, 1.0f, "%.3f s");
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("Still allowed to jump this long after walking off a ledge");
            PropertyFloat("Jump Buffer", &cc.jumpBuffer, 0.005f, 0.0f, 1.0f, "%.3f s");
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("A jump pressed this long before landing still happens");
        }

        SectionLabel("Collision");
        PropertyText("Solid Tag", cc.solidTag);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Only collide with Collision components on entities with this tag. Empty = every Collision component (triggers are ignored)");
        if (platformer)
        {
            PropertyText("One Way Tag", cc.oneWayTag);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("Platforms with this tag only block from above");
        }
        PropertyBool("Solid Tilemaps", &cc.collideTilemaps);
        PropertyBool("Flip Sprite", &cc.flipSprite);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Mirror the sprite to face the movement direction");

        if (!ctx.IsEditing())
        {
            char state[96];
            std::snprintf(state, sizeof(state), "%s  vel %.0f, %.0f", platformer ? (cc.grounded ? "grounded" : "airborne") : "moving",
                          cc.velocity.x, cc.velocity.y);
            PropertyValue("State", state);
        }
        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextWrapped(platformer ? "Scripts receive OnJump and OnLand(speed)" : "Moves in 8 directions and slides along walls");
        ImGui::PopStyleColor(); });

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

    DrawComponentSection<DamageComponent>(ctx, "Damage", m_SelectedEntity, [&m_SelectedEntity](DamageComponent &damage)
                                          {
        PropertyBool("Enabled", &damage.enabled);
        PropertyFloat("Amount", &damage.amount, 0.5f, -1000000.0f, 1000000.0f, "%.1f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Health taken from whatever enters the trigger zone. Negative heals (pickups)");
        PropertyFloat("Repeat Every", &damage.interval, 0.05f, 0.0f, 1000.0f, "%.2f s");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Hit again this often while the target stays inside. 0 = once per entry");
        PropertyText("Only Entity", damage.filter);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Leave empty to hit everything with a Health component, or type an entity name");
        PropertyFloat("Knockback", &damage.knockback, 5.0f, 0.0f, 100000.0f, "%.0f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Pushes Character Controllers away from this entity");
        PropertyBool("Destroy On Hit", &damage.destroyOnHit);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Remove this entity after its first hit (projectiles, pickups)");
        if (!m_SelectedEntity.HasComponent<TriggerComponent>())
        {
            PropertyLabel("");
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::Warning);
            ImGui::TextWrapped("Needs a Trigger Zone on this entity to find targets");
            ImGui::PopStyleColor();
        } });

    DrawComponentSection<OscillatorComponent>(ctx, "Oscillator", m_SelectedEntity, [&ctx](OscillatorComponent &osc)
                                              {
        PropertyVec2("Move", &osc.move.x, 0.5f, "%.1f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("How far it bobs from where it starts, in pixels");
        PropertyFloat("Frequency", &osc.frequency, 0.01f, 0.0f, 100.0f, "%.2f Hz");
        PropertyFloat("Phase", &osc.phase, 0.01f, 0.0f, 1.0f, "%.2f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Offset copies by different phases so they don't move in lockstep");
        PropertyFloat("Swing", &osc.swing, 0.5f, 0.0f, 180.0f, "%.1f deg");
        PropertyFloat("Spin", &osc.spin, 1.0f, -100000.0f, 100000.0f, "%.0f deg/s");
        PropertyFloat("Pulse", &osc.pulse, 0.005f, 0.0f, 0.95f, "%.2f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Scale breathing. 0.1 = grows and shrinks by 10%%");
        PropertyBool("Autostart", &osc.playing);
        if (ctx.IsEditing())
        {
            PropertyLabel("");
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
            ImGui::TextWrapped("Moves while the game runs");
            ImGui::PopStyleColor();
        } });

    DrawComponentSection<SpawnerComponent>(ctx, "Spawner", m_SelectedEntity, [&ctx](SpawnerComponent &spawner)
                                           {
        PropertyText("Prototype", spawner.prototype);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Name of the entity to copy. Drag one here from the Scene panel");
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY"); payload && payload->DataSize == sizeof(entt::entity))
            {
                const entt::entity dropped = *static_cast<const entt::entity *>(payload->Data);
                if (const auto *tag = ctx.Scene->GetRegistry().try_get<TagComponent>(dropped))
                {
                    EditorCommands::RecordEdit(ctx);
                    spawner.prototype = tag->Tag;
                }
            }
            ImGui::EndDragDropTarget();
        }
        PropertyBool("Hide Prototype", &spawner.hidePrototype);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Remove the original from the running game so only the copies appear");

        SectionLabel("Timing");
        PropertyFloat("Interval", &spawner.interval, 0.05f, 0.01f, 100000.0f, "%.2f s");
        PropertyInt("Per Spawn", &spawner.perSpawn, 0.1f, 1, 1000);
        PropertyInt("Max Alive", &spawner.maxAlive, 0.1f, 0, 100000);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Stop spawning while this many copies exist. 0 = no limit");
        PropertyInt("Total Limit", &spawner.totalLimit, 0.1f, 0, 1000000);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Copies made over the whole game. 0 = no limit");
        PropertyBool("Spawn On Start", &spawner.spawnOnStart);
        PropertyBool("Autostart", &spawner.playing);

        SectionLabel("Placement");
        PropertyVec2("Offset", &spawner.offset.x, 0.5f, "%.1f");
        PropertyVec2("Random Spread", &spawner.spread.x, 0.5f, "%.1f", 0.0f, 100000.0f);
        PropertyVec2("Velocity", &spawner.velocity.x, 1.0f, "%.1f");
        PropertyFloat("Lifetime", &spawner.lifetime, 0.05f, 0.0f, 100000.0f, "%.2f s");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Copies remove themselves after this long. 0 = keep them");

        if (!ctx.IsEditing())
        {
            char state[64];
            std::snprintf(state, sizeof(state), "%zu alive, %d spawned", spawner.alive.size(), spawner.spawned);
            PropertyValue("State", state);
        }
        else if (!spawner.prototype.empty() && !ctx.Scene->GetEntityByName(spawner.prototype))
        {
            PropertyLabel("");
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::Warning);
            ImGui::TextWrapped("No entity is named \"%s\"", spawner.prototype.c_str());
            ImGui::PopStyleColor();
        } });
}
