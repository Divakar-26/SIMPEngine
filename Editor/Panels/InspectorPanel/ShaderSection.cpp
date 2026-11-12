#include "InspectorPanel.h"

#include <Engine/Rendering/ShaderEffect.h>
#include <Engine/Scene/Systems/ShaderCanvasSystem.h>

void InspectorPanel::DrawShaderSection(SIMPEngine::Entity &m_SelectedEntity)
{
    using namespace EditorUI;

    EditorContext &ctx = *m_Context;

    DrawComponentSection<ShaderCanvasComponent>(ctx, "Shader Canvas", m_SelectedEntity, [&ctx](ShaderCanvasComponent &canvas)
                                                {
        PropertyBool("Enabled", &canvas.enabled);
        PropertyText("Shader", canvas.shader);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Drag a .glsl file here from the FileSystem panel");
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                std::string path(static_cast<const char *>(payload->Data));
                if (LowerExtension(path) == ".glsl")
                {
                    EditorCommands::RecordEdit(ctx);
                    canvas.shader = path;
                }
            }
            ImGui::EndDragDropTarget();
        }

        PropertyLabel("");
        const float half = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
        if (Button("New Shader", ButtonKind::Secondary, ImVec2(half, 0)))
        {
            const std::string created = EditorCommands::CreateShaderFile("Effect");
            if (created.empty())
                LOG_ERROR("Could not create a shader file under assets://shaders");
            else
            {
                EditorCommands::RecordEdit(ctx);
                canvas.shader = created;
                ctx.OpenScriptRequest = created;
            }
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(canvas.shader.empty());
        if (Button("Edit", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
            ctx.OpenScriptRequest = canvas.shader;
        ImGui::EndDisabled();

        if (!canvas.error.empty())
        {
            PropertyLabel("");
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::Error);
            ImGui::TextWrapped("%s", canvas.error.c_str());
            ImGui::PopStyleColor();
        }

        static const char *scopes[] = {"Scene", "Children", "Z Range"};
        PropertySegmented("Scope", &canvas.scope, scopes, 3);
        if (canvas.scope == 2)
            PropertyRange("Z Range", &canvas.zMin, 0.1f, "%.2f");
        PropertyBool("Limit To Area", &canvas.limitArea);
        if (canvas.limitArea)
        {
            PropertyVec2("Area Size", &canvas.areaSize.x, 1.0f, "%.0f", 1.0f, 100000.0f);
            PropertyLabel("");
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
            ImGui::TextWrapped("The area is centred on this entity and also follows its Scale, so you can move and scale it in the Viewport with the gizmos");
            ImGui::PopStyleColor();
        }
        PropertyInt("Order", &canvas.order, 1.0f, -100, 100);
        if (canvas.scope != 0)
            PropertyFloat("Resolution Scale", &canvas.resolutionScale, 0.01f, 0.1f, 2.0f, "%.2f");
        PropertyFloat("Progress", &canvas.progress, 0.005f, 0.0f, 1.0f, "%.3f");

        if (!canvas.uniforms.empty())
        {
            PropertyLabel("");
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
            ImGui::TextUnformatted("Uniforms");
            ImGui::PopStyleColor();
        }
        for (ShaderUniformValue &uniform : canvas.uniforms)
        {
            ImGui::PushID(uniform.name.c_str());
            const char *label = uniform.name.c_str();
            switch (static_cast<SIMPEngine::ShaderUniformType>(uniform.type))
            {
            case SIMPEngine::ShaderUniformType::Float:
            {
                const bool ranged = uniform.max > uniform.min;
                PropertyFloat(label, &uniform.value[0], ranged ? (uniform.max - uniform.min) / 200.0f : 0.01f, ranged ? uniform.min : 0.0f, ranged ? uniform.max : 0.0f, "%.3f");
                break;
            }
            case SIMPEngine::ShaderUniformType::Vec2:
                PropertyVec2(label, uniform.value, 0.01f, "%.3f");
                break;
            case SIMPEngine::ShaderUniformType::Vec3:
                PropertyLabel(label);
                if (ImGui::DragFloat3("##v3", uniform.value, 0.01f))
                    NotifyEdit();
                break;
            case SIMPEngine::ShaderUniformType::Vec4:
                PropertyLabel(label);
                if (ImGui::DragFloat4("##v4", uniform.value, 0.01f))
                    NotifyEdit();
                break;
            case SIMPEngine::ShaderUniformType::Color:
                PropertyColor(label, uniform.value);
                break;
            case SIMPEngine::ShaderUniformType::Int:
            {
                int value = static_cast<int>(uniform.value[0]);
                if (PropertyInt(label, &value, 1.0f, static_cast<int>(uniform.min), static_cast<int>(uniform.max)))
                    uniform.value[0] = static_cast<float>(value);
                break;
            }
            case SIMPEngine::ShaderUniformType::Bool:
            {
                bool value = uniform.value[0] != 0.0f;
                if (PropertyBool(label, &value))
                    uniform.value[0] = value ? 1.0f : 0.0f;
                break;
            }
            case SIMPEngine::ShaderUniformType::Texture:
            {
                std::string path = uniform.texturePath;
                if (PropertyText(label, path))
                {
                    uniform.texturePath = path;
                    uniform.texture = path.empty() ? nullptr : EditorCommands::LoadTexture(path);
                }
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("TEXTURE_ASSET"))
                    {
                        EditorCommands::RecordEdit(ctx);
                        uniform.texturePath = static_cast<const char *>(payload->Data);
                        uniform.texture = EditorCommands::LoadTexture(uniform.texturePath);
                    }
                    ImGui::EndDragDropTarget();
                }
                break;
            }
            }
            ImGui::PopID();
        }

        PropertyLabel("");
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextWrapped("The viewport shows the effect while the lighting preview is on. Scene scope runs over everything before the UI; Children and Z Range draw only those entities through the shader");
        ImGui::PopStyleColor(); });
}
