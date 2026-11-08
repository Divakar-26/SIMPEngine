#include "EditorHistory.h"

#include <Engine/Scene/Component.h>
#include <Engine/Scene/SceneSerializer.h>
#include <Engine/Scene/ScriptableEntity.h>

#include <algorithm>

namespace
{
    std::vector<entt::entity> OrderedEntities(SIMPEngine::Scene &scene)
    {
        std::vector<entt::entity> entities;
        for (auto handle : scene.GetRegistry().view<TagComponent>())
            entities.push_back(handle);
        std::reverse(entities.begin(), entities.end());
        return entities;
    }
}

EditorHistory::EditorHistory(EditorContext *context)
    : m_Context(context)
{
}

SceneSnapshot EditorHistory::Capture() const
{
    SceneSnapshot snapshot;
    SIMPEngine::Scene &scene = *m_Context->Scene;
    SIMPEngine::SceneSerializer serializer(&scene);
    snapshot.Yaml = serializer.SerializeToString();

    auto entities = OrderedEntities(scene);
    auto &registry = scene.GetRegistry();
    for (size_t i = 0; i < entities.size(); ++i)
    {
        if (auto *sc = registry.try_get<ScriptComponent>(entities[i]); sc && sc->InstantiateScript && sc->LuaPath.empty())
            snapshot.Scripts.emplace_back(i, sc->InstantiateScript);
        if (m_Context->IsSelected(entities[i]))
            snapshot.SelectedIndex = static_cast<int>(i);
    }
    return snapshot;
}

void EditorHistory::Restore(const SceneSnapshot &snapshot, int selectIndex)
{
    SIMPEngine::Scene &scene = *m_Context->Scene;
    m_Context->SelectedEntity = {};
    m_Context->HoveredEntity = {};

    scene.Clear();

    std::vector<entt::entity> created;
    SIMPEngine::SceneSerializer serializer(&scene);
    serializer.DeserializeFromString(snapshot.Yaml, &created);

    auto &registry = scene.GetRegistry();
    for (const auto &[index, factory] : snapshot.Scripts)
    {
        if (index >= created.size() || !registry.valid(created[index]))
            continue;
        auto &sc = registry.emplace_or_replace<ScriptComponent>(created[index]);
        sc.InstantiateScript = factory;
    }

    int index = selectIndex == -2 ? snapshot.SelectedIndex : selectIndex;
    if (index >= 0 && static_cast<size_t>(index) < created.size())
        m_Context->SelectedEntity = SIMPEngine::Entity(created[index], &scene);
}

void EditorHistory::BeginEdit()
{
    if (!m_Context->IsEditing())
        return;

    m_Context->Dirty = true;
    if (m_EditOpen)
        return;

    if (!m_StableValid)
        RefreshStable();

    m_Undo.push_back(m_Stable);
    if (m_Undo.size() > MaxSteps)
        m_Undo.erase(m_Undo.begin());
    m_Redo.clear();
    m_EditOpen = true;
}

void EditorHistory::Commit()
{
    if (!m_EditOpen)
        return;
    m_EditOpen = false;
    RefreshStable();
}

void EditorHistory::EndFrame(bool interactionActive)
{
    if (m_EditOpen && !interactionActive)
        Commit();
}

void EditorHistory::RefreshStable()
{
    m_Stable = Capture();
    m_StableValid = true;
}

void EditorHistory::Clear()
{
    m_Undo.clear();
    m_Redo.clear();
    m_EditOpen = false;
    RefreshStable();
}

void EditorHistory::Undo()
{
    if (!m_Context->IsEditing() || m_Undo.empty())
        return;
    Commit();

    SceneSnapshot current = Capture();
    int selection = current.SelectedIndex;
    m_Redo.push_back(std::move(current));
    SceneSnapshot target = std::move(m_Undo.back());
    m_Undo.pop_back();
    Restore(target, selection);
    m_Stable = std::move(target);
    m_StableValid = true;
    m_Context->Dirty = true;
}

void EditorHistory::Redo()
{
    if (!m_Context->IsEditing() || m_Redo.empty())
        return;
    Commit();

    SceneSnapshot current = Capture();
    int selection = current.SelectedIndex;
    m_Undo.push_back(std::move(current));
    SceneSnapshot target = std::move(m_Redo.back());
    m_Redo.pop_back();
    Restore(target, selection);
    m_Stable = std::move(target);
    m_StableValid = true;
    m_Context->Dirty = true;
}
