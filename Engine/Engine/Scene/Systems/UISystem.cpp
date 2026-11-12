#include "UISystem.h"

#include <Engine/Audio/AudioEngine.h>
#include <Engine/Core/GameVariables.h>
#include <Engine/Input/Input.h>
#include <Engine/Input/InputActions.h>
#include <Engine/Rendering/Font.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Scene/Scene.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace SIMPEngine
{
    namespace
    {
        constexpr float BaseZ = 100.0f;
        constexpr float ZStep = 0.01f;

        bool CanvasShown(const CanvasComponent &canvas)
        {
            return canvas.visible;
        }

        bool Contains(const glm::vec4 &rect, const glm::vec2 &point)
        {
            return point.x >= rect.x && point.y >= rect.y && point.x <= rect.x + rect.z && point.y <= rect.y + rect.w;
        }

        glm::vec4 Resolve(const UIRectComponent &node, const glm::vec4 &parent)
        {
            const float pw = parent.z, ph = parent.w;
            float x, width;
            if (node.anchorMin.x != node.anchorMax.x)
            {
                const float left = parent.x + node.anchorMin.x * pw + node.margin.x;
                const float right = parent.x + node.anchorMax.x * pw - node.margin.z;
                x = left;
                width = std::max(0.0f, right - left);
            }
            else
            {
                width = node.size.x;
                x = parent.x + node.anchorMin.x * pw + node.position.x - node.pivot.x * width;
            }
            float y, height;
            if (node.anchorMin.y != node.anchorMax.y)
            {
                const float bottom = parent.y + node.anchorMin.y * ph + node.margin.y;
                const float top = parent.y + node.anchorMax.y * ph - node.margin.w;
                y = bottom;
                height = std::max(0.0f, top - bottom);
            }
            else
            {
                height = node.size.y;
                y = parent.y + node.anchorMin.y * ph + node.position.y - node.pivot.y * height;
            }
            return {x, y, width, height};
        }

        void LayoutChildren(entt::registry &registry, entt::entity entity, const glm::vec4 &rect);

        void LayoutNode(entt::registry &registry, entt::entity entity, const glm::vec4 &parent)
        {
            auto *node = registry.try_get<UIRectComponent>(entity);
            if (!node)
                return;
            node->rect = Resolve(*node, parent);
            LayoutChildren(registry, entity, node->rect);
        }

        void Arrange(entt::registry &registry, const UILayoutComponent &layout, const std::vector<entt::entity> &items, const glm::vec4 &rect)
        {
            const float left = rect.x + layout.padding.x;
            const float bottom = rect.y + layout.padding.y;
            const float width = std::max(0.0f, rect.z - layout.padding.x - layout.padding.z);
            const float height = std::max(0.0f, rect.w - layout.padding.y - layout.padding.w);
            const bool column = layout.direction == 1;

            float total = 0.0f;
            for (entt::entity item : items)
            {
                const UIRectComponent &node = registry.get<UIRectComponent>(item);
                total += column ? node.size.y : node.size.x;
            }
            if (items.size() > 1)
                total += layout.spacing * static_cast<float>(items.size() - 1);

            const float room = column ? height : width;
            float offset = 0.0f;
            if (layout.justify == 1)
                offset = (room - total) * 0.5f;
            else if (layout.justify == 2)
                offset = room - total;

            float cursor = offset;
            for (entt::entity item : items)
            {
                UIRectComponent &node = registry.get<UIRectComponent>(item);
                float w = node.size.x, h = node.size.y;
                if (layout.stretch)
                {
                    if (column)
                        w = width;
                    else
                        h = height;
                }

                float x, y;
                if (column)
                {
                    y = bottom + height - cursor - h;
                    x = layout.align == 0 ? left : layout.align == 2 ? left + width - w
                                                                      : left + (width - w) * 0.5f;
                    cursor += h + layout.spacing;
                }
                else
                {
                    x = left + cursor;
                    y = layout.align == 0 ? bottom + height - h : layout.align == 2 ? bottom
                                                                                    : bottom + (height - h) * 0.5f;
                    cursor += w + layout.spacing;
                }
                node.rect = {x, y, w, h};
                LayoutChildren(registry, item, node.rect);
            }
        }

        void LayoutChildren(entt::registry &registry, entt::entity entity, const glm::vec4 &rect)
        {
            auto *hierarchy = registry.try_get<HierarchyComponent>(entity);
            if (!hierarchy)
                return;
            const std::vector<entt::entity> children = hierarchy->children;

            const UILayoutComponent *layout = registry.try_get<UILayoutComponent>(entity);
            if (layout && layout->enabled && entity != entt::null && registry.all_of<UIRectComponent>(entity))
            {
                std::vector<entt::entity> items;
                for (entt::entity child : children)
                    if (registry.valid(child) && registry.all_of<UIRectComponent>(child) && registry.get<UIRectComponent>(child).visible)
                        items.push_back(child);
                Arrange(registry, *layout, items, rect);
                return;
            }
            for (entt::entity child : children)
                if (registry.valid(child))
                    LayoutNode(registry, child, rect);
        }

        void LayoutCanvasRoot(entt::registry &registry, entt::entity entity)
        {
            const CanvasComponent &canvas = registry.get<CanvasComponent>(entity);
            const float inset = std::clamp(canvas.safeArea, 0.0f, 0.4f) * std::min(canvas.size.x, canvas.size.y);
            LayoutChildren(registry, entity, {inset, inset, std::max(0.0f, canvas.size.x - inset * 2.0f), std::max(0.0f, canvas.size.y - inset * 2.0f)});
        }

        SDL_Color Multiply(SDL_Color color, const glm::vec4 &tint)
        {
            auto channel = [](uint8_t value, float factor)
            { return static_cast<uint8_t>(std::clamp(value * factor, 0.0f, 255.0f)); };
            return {channel(color.r, tint.r), channel(color.g, tint.g), channel(color.b, tint.b), channel(color.a, tint.a)};
        }

        glm::vec4 ToVec(SDL_Color color)
        {
            return {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
        }

        std::vector<entt::entity> SortedCanvases(entt::registry &registry)
        {
            std::vector<entt::entity> canvases;
            for (entt::entity entity : registry.view<CanvasComponent>())
                canvases.push_back(entity);
            std::stable_sort(canvases.begin(), canvases.end(), [&](entt::entity a, entt::entity b)
                             { return registry.get<CanvasComponent>(a).sortOrder < registry.get<CanvasComponent>(b).sortOrder; });
            return canvases;
        }

        void CollectNode(entt::registry &registry, entt::entity entity, entt::entity canvas, std::vector<std::pair<entt::entity, entt::entity>> &out)
        {
            auto *node = registry.try_get<UIRectComponent>(entity);
            if (!node || !node->visible)
                return;
            out.emplace_back(entity, canvas);
            if (auto *hierarchy = registry.try_get<HierarchyComponent>(entity))
            {
                const std::vector<entt::entity> children = hierarchy->children;
                for (entt::entity child : children)
                    if (registry.valid(child))
                        CollectNode(registry, child, canvas, out);
            }
        }
    }

    glm::vec2 UISystem::ToCanvas(const CanvasComponent &canvas, const glm::vec2 &point, const glm::vec2 &viewport)
    {
        if (viewport.x <= 0.0f || viewport.y <= 0.0f)
            return {0.0f, 0.0f};
        return {point.x / viewport.x * canvas.size.x, canvas.size.y - point.y / viewport.y * canvas.size.y};
    }

    void UISystem::Layout(entt::registry &registry, const glm::vec2 &viewport)
    {
        for (entt::entity entity : registry.view<CanvasComponent>())
        {
            CanvasComponent &canvas = registry.get<CanvasComponent>(entity);
            if (viewport.x <= 0.0f || viewport.y <= 0.0f)
                continue;
            const float refW = std::max(1.0f, canvas.referenceSize.x);
            const float refH = std::max(1.0f, canvas.referenceSize.y);
            if (canvas.scaleMode == 0)
                canvas.scale = 1.0f;
            else
            {
                const float lw = std::log2(viewport.x / refW);
                const float lh = std::log2(viewport.y / refH);
                canvas.scale = std::exp2(lw + (lh - lw) * std::clamp(canvas.match, 0.0f, 1.0f));
            }
            canvas.size = viewport / std::max(canvas.scale, 0.0001f);
            LayoutCanvasRoot(registry, entity);
        }
    }

    void UISystem::Gather(entt::registry &registry, std::vector<Node> &nodes, std::vector<entt::entity> &canvases)
    {
        canvases = SortedCanvases(registry);
        for (entt::entity canvas : canvases)
        {
            if (!CanvasShown(registry.get<CanvasComponent>(canvas)))
                continue;
            std::vector<std::pair<entt::entity, entt::entity>> found;
            if (auto *hierarchy = registry.try_get<HierarchyComponent>(canvas))
            {
                const std::vector<entt::entity> children = hierarchy->children;
                for (entt::entity child : children)
                    if (registry.valid(child))
                        CollectNode(registry, child, canvas, found);
            }
            for (auto &[entity, owner] : found)
                nodes.push_back({entity, owner});
        }
    }

    void UISystem::Clear()
    {
        m_MouseOverUI = false;
        m_FocusVisible = false;
        m_Focus = entt::null;
        m_LastMouse = {-1.0f, -1.0f};
    }

    void UISystem::Navigate(entt::registry &registry, const std::vector<Node> &nodes, const glm::vec2 &direction)
    {
        std::vector<entt::entity> buttons;
        for (const Node &node : nodes)
        {
            auto *button = registry.try_get<UIButtonComponent>(node.Entity);
            if (button && button->enabled && button->focusable)
                buttons.push_back(node.Entity);
        }
        if (buttons.empty())
            return;

        auto center = [&](entt::entity entity)
        {
            const glm::vec4 &r = registry.get<UIRectComponent>(entity).rect;
            return glm::vec2(r.x + r.z * 0.5f, r.y + r.w * 0.5f);
        };

        const bool valid = m_Focus != entt::null && registry.valid(m_Focus) && std::find(buttons.begin(), buttons.end(), m_Focus) != buttons.end();
        if (!valid || !m_FocusVisible)
        {
            if (!valid)
            {
                entt::entity best = buttons.front();
                for (entt::entity entity : buttons)
                {
                    const glm::vec2 c = center(entity), b = center(best);
                    if (c.y > b.y + 1.0f || (std::abs(c.y - b.y) <= 1.0f && c.x < b.x))
                        best = entity;
                }
                m_Focus = best;
            }
            m_FocusVisible = true;
            return;
        }

        const glm::vec2 from = center(m_Focus);
        entt::entity best = entt::null;
        float bestScore = 0.0f;
        for (entt::entity entity : buttons)
        {
            if (entity == m_Focus)
                continue;
            const glm::vec2 delta = center(entity) - from;
            const float along = glm::dot(delta, direction);
            if (along <= 1.0f)
                continue;
            const float across = std::abs(delta.x * direction.y - delta.y * direction.x);
            const float score = along + across * 2.0f;
            if (best == entt::null || score < bestScore)
            {
                best = entity;
                bestScore = score;
            }
        }
        if (best != entt::null)
            m_Focus = best;
    }

    void UISystem::Update(Scene &scene, float)
    {
        entt::registry &registry = scene.GetRegistry();
        if (registry.view<CanvasComponent>().empty())
        {
            m_MouseOverUI = false;
            m_Focus = entt::null;
            return;
        }

        const glm::vec2 inputViewport = Input::GetViewportSize();
        glm::vec2 viewport = inputViewport;
        {
            const auto [cameraWidth, cameraHeight] = scene.GetActiveCamera().GetViewportSize();
            if (cameraWidth > 0 && cameraHeight > 0)
                viewport = {static_cast<float>(cameraWidth), static_cast<float>(cameraHeight)};
        }
        Layout(registry, viewport);

        std::vector<Node> nodes;
        std::vector<entt::entity> canvases;
        Gather(registry, nodes, canvases);

        const bool inside = Input::IsMouseInViewport();
        glm::vec2 mousePoint = Input::GetMouseViewportPosition();
        if (inputViewport.x > 0.0f && inputViewport.y > 0.0f)
            mousePoint = {mousePoint.x * viewport.x / inputViewport.x, mousePoint.y * viewport.y / inputViewport.y};

        m_MouseOverUI = false;
        entt::entity hoveredButton = entt::null;
        if (inside)
        {
            for (auto it = nodes.rbegin(); it != nodes.rend(); ++it)
            {
                const UIRectComponent &rect = registry.get<UIRectComponent>(it->Entity);
                if (!rect.blocksInput)
                    continue;
                const bool solid = registry.all_of<UIPanelComponent>(it->Entity) || registry.all_of<UIImageComponent>(it->Entity) || registry.all_of<UIButtonComponent>(it->Entity);
                if (!solid)
                    continue;
                const glm::vec2 point = ToCanvas(registry.get<CanvasComponent>(it->Canvas), mousePoint, viewport);
                if (!Contains(rect.rect, point))
                    continue;
                m_MouseOverUI = true;
                entt::entity cursor = it->Entity;
                while (cursor != entt::null && registry.valid(cursor))
                {
                    if (registry.all_of<UIButtonComponent>(cursor))
                    {
                        hoveredButton = cursor;
                        break;
                    }
                    auto *hierarchy = registry.try_get<HierarchyComponent>(cursor);
                    cursor = hierarchy ? hierarchy->parent : entt::null;
                }
                break;
            }
        }

        const bool mouseMoved = mousePoint != m_LastMouse;
        m_LastMouse = mousePoint;
        if (mouseMoved && inside)
        {
            m_FocusVisible = false;
            if (hoveredButton != entt::null)
                m_Focus = hoveredButton;
        }

        if (InputActions::IsJustPressed("UIFocus"))
        {
            if (m_FocusVisible)
                m_FocusVisible = false;
            else
                Navigate(registry, nodes, glm::vec2(0.0f));
        }

        float navX = 0.0f, navY = 0.0f;
        if (m_FocusVisible && InputActions::IsJustPressed("MoveX"))
            navX = InputActions::GetAxis("MoveX") > 0.0f ? 1.0f : -1.0f;
        if (m_FocusVisible && InputActions::IsJustPressed("MoveY"))
            navY = InputActions::GetAxis("MoveY") > 0.0f ? 1.0f : -1.0f;
        if (navX != 0.0f || navY != 0.0f)
            Navigate(registry, nodes, navX != 0.0f ? glm::vec2(navX, 0.0f) : glm::vec2(0.0f, navY));

        const bool confirm = m_FocusVisible && InputActions::IsJustPressed("UIConfirm");

        struct Pending
        {
            entt::entity Entity;
            ScriptEvent Type;
        };
        std::vector<Pending> pending;

        for (const Node &node : nodes)
        {
            auto *button = registry.try_get<UIButtonComponent>(node.Entity);
            if (!button)
                continue;
            const bool hovered = button->enabled && node.Entity == hoveredButton;
            if (hovered && !button->hovering)
            {
                pending.push_back({node.Entity, ScriptEvent::HoverEnter});
                if (!button->hoverSound.empty())
                    AudioEngine::Play(button->hoverSound, button->volume, 1.0f, false, AudioBus::Ui);
            }
            else if (!hovered && button->hovering)
                pending.push_back({node.Entity, ScriptEvent::HoverExit});
            button->hovering = hovered;

            if (hovered && Input::IsMouseButtonJustPressed(1))
                button->pressed = true;
            if (button->pressed && (!button->enabled || !Input::IsMouseButtonPressed(1)))
            {
                button->pressed = false;
                if (hovered)
                {
                    pending.push_back({node.Entity, ScriptEvent::Clicked});
                    if (!button->clickSound.empty())
                        AudioEngine::Play(button->clickSound, button->volume, 1.0f, false, AudioBus::Ui);
                }
            }

            button->focused = m_FocusVisible && node.Entity == m_Focus && button->enabled;
            if (confirm && button->focused)
            {
                pending.push_back({node.Entity, ScriptEvent::Clicked});
                if (!button->clickSound.empty())
                    AudioEngine::Play(button->clickSound, button->volume, 1.0f, false, AudioBus::Ui);
            }
        }

        for (const Pending &event : pending)
            scene.SendEvent(event.Entity, event.Type);
    }

    void UISystem::DrawNode(entt::registry &registry, entt::entity entity, const glm::vec4 &inheritedTint, float &z)
    {
        auto *node = registry.try_get<UIRectComponent>(entity);
        if (!node || !node->visible)
            return;

        glm::vec4 tint = inheritedTint;
        float scale = 1.0f;
        auto *button = registry.try_get<UIButtonComponent>(entity);
        if (button)
        {
            if (!button->enabled)
                tint *= ToVec(button->disabledTint);
            else if (button->pressed)
            {
                tint *= ToVec(button->pressTint);
                scale = button->pressScale;
            }
            else if (button->hovering || button->focused)
            {
                tint *= ToVec(button->hoverTint);
                scale = button->hoverScale;
            }
        }

        const glm::vec4 &r = node->rect;
        const float cx = r.x + r.z * 0.5f;
        const float cy = r.y + r.w * 0.5f;
        const float hw = r.z * 0.5f * scale;
        const float hh = r.w * 0.5f * scale;

        if (auto *panel = registry.try_get<UIPanelComponent>(entity))
        {
            if (panel->borderWidth > 0.0f && panel->borderColor.a > 0)
            {
                Renderer::DrawRoundedRect(cx, cy, hw, hh, panel->cornerRadius + panel->borderWidth, Multiply(panel->borderColor, tint), 1.0f, z);
                z += ZStep;
                const float bw = panel->borderWidth;
                Renderer::DrawRoundedRect(cx, cy, std::max(0.0f, hw - bw), std::max(0.0f, hh - bw), panel->cornerRadius, Multiply(panel->color, tint), 1.0f, z);
            }
            else if (panel->cornerRadius > 0.0f)
                Renderer::DrawRoundedRect(cx, cy, hw, hh, panel->cornerRadius, Multiply(panel->color, tint), 1.0f, z);
            else
                Renderer::DrawQuad(cx, cy, hw * 2.0f, hh * 2.0f, 0.0f, Multiply(panel->color, tint), true, z);
            z += ZStep;
        }

        if (auto *image = registry.try_get<UIImageComponent>(entity); image && image->texture)
        {
            glm::vec2 size{hw * 2.0f, hh * 2.0f};
            if (image->preserveAspect)
            {
                float texW = static_cast<float>(image->texture->GetWidth());
                float texH = static_cast<float>(image->texture->GetHeight());
                if (image->useRegion && image->region.w > 0.0f && image->region.h > 0.0f)
                {
                    texW = image->region.w;
                    texH = image->region.h;
                }
                if (texW > 0.0f && texH > 0.0f)
                {
                    const float fit = std::min(size.x / texW, size.y / texH);
                    size = {texW * fit, texH * fit};
                }
            }
            SpriteDesc desc;
            desc.TextureRef = image->texture;
            desc.Position = {cx, cy};
            desc.Size = size;
            desc.Z = z;
            desc.Tint = Multiply(image->tint, tint);
            desc.SourceRect = image->useRegion ? &image->region : nullptr;
            Renderer::DrawSprite(desc);
            z += ZStep;
        }

        if (auto *label = registry.try_get<UILabelComponent>(entity); label && !label->text.empty())
        {
            const std::string shown = GameVariables::Format(label->text);
            auto font = label->fontPath.empty() ? Font::GetDefault() : Font::Load(label->fontPath);
            if (font)
            {
                const glm::vec2 extent = font->MeasureText(shown, label->size);
                const float x = label->align == 0 ? r.x : label->align == 2 ? r.x + r.z
                                                                           : cx;
                const float top = label->verticalAlign == 0 ? r.y + r.w : label->verticalAlign == 2 ? r.y + extent.y
                                                                                                     : cy + extent.y * 0.5f;
                const TextAlign align = label->align == 0 ? TextAlign::Left : label->align == 2 ? TextAlign::Right
                                                                                                : TextAlign::Center;
                if (label->shadow)
                {
                    Renderer::DrawText(shown, x + 2.0f, top - 2.0f, label->size, Multiply({0, 0, 0, static_cast<uint8_t>(label->color.a * 0.6f)}, tint), align, 0.0f, z, font.get());
                    z += ZStep;
                }
                Renderer::DrawText(shown, x, top, label->size, Multiply(label->color, tint), align, 0.0f, z, font.get());
                z += ZStep;
            }
        }

        if (button && button->focused)
        {
            Renderer::DrawRect(cx, cy, hw * 2.0f + 6.0f, hh * 2.0f + 6.0f, 2.0f, {255, 220, 90, 255}, 0.0f, z);
            z += ZStep;
        }

        if (auto *hierarchy = registry.try_get<HierarchyComponent>(entity))
        {
            const std::vector<entt::entity> children = hierarchy->children;
            for (entt::entity child : children)
                if (registry.valid(child))
                    DrawNode(registry, child, tint, z);
        }
    }

    void UISystem::Render(Scene &scene, bool screenSpace)
    {
        entt::registry &registry = scene.GetRegistry();
        if (registry.view<CanvasComponent>().empty())
            return;

        glm::mat4 previousView = Renderer::GetViewMatrix();
        glm::mat4 previousProjection = Renderer::GetAPI()->GetProjectionMatrix();

        if (screenSpace)
        {
            const glm::vec2 viewport = Renderer::GetViewportSize();
            if (viewport.x <= 0.0f || viewport.y <= 0.0f)
                return;
            Layout(registry, viewport);
        }
        else
        {
            for (entt::entity entity : registry.view<CanvasComponent>())
            {
                CanvasComponent &canvas = registry.get<CanvasComponent>(entity);
                canvas.scale = 1.0f;
                canvas.size = canvas.referenceSize;
            }
            for (entt::entity entity : registry.view<CanvasComponent>())
                LayoutCanvasRoot(registry, entity);
        }

        std::vector<entt::entity> canvases = SortedCanvases(registry);

        Renderer::Flush();
        if (screenSpace)
        {
            const CanvasComponent &first = registry.get<CanvasComponent>(canvases.front());
            Renderer::SetViewMatrix(glm::mat4(1.0f));
            Renderer::SetProjectionMatrix(glm::ortho(0.0f, first.size.x, 0.0f, first.size.y, -1000.0f, 1000.0f));
        }

        float z = BaseZ;
        for (entt::entity canvasEntity : canvases)
        {
            const CanvasComponent &canvas = registry.get<CanvasComponent>(canvasEntity);
            if (!canvas.visible)
                continue;

            if (screenSpace)
            {
                Renderer::Flush();
                Renderer::SetProjectionMatrix(glm::ortho(0.0f, canvas.size.x, 0.0f, canvas.size.y, -1000.0f, 1000.0f));
            }
            else
            {
                Renderer::DrawRect(canvas.size.x * 0.5f, canvas.size.y * 0.5f, canvas.size.x, canvas.size.y, Renderer::GetWorldPerPixel() * 1.5f, {120, 170, 255, 160}, 0.0f, z);
                z += ZStep;
            }

            if (auto *hierarchy = registry.try_get<HierarchyComponent>(canvasEntity))
            {
                const std::vector<entt::entity> children = hierarchy->children;
                for (entt::entity child : children)
                    if (registry.valid(child))
                        DrawNode(registry, child, glm::vec4(1.0f), z);
            }
        }

        Renderer::Flush();
        Renderer::SetViewMatrix(previousView);
        Renderer::SetProjectionMatrix(previousProjection);
    }
}
