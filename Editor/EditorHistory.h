#pragma once

#include "EditorContext.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace SIMPEngine
{
    class ScriptableEntity;
}

struct SceneSnapshot
{
    using ScriptFactory = std::function<std::unique_ptr<SIMPEngine::ScriptableEntity>()>;

    std::string Yaml;
    std::vector<std::pair<size_t, ScriptFactory>> Scripts;
    int SelectedIndex = -1;
};

class EditorHistory
{
public:
    explicit EditorHistory(EditorContext *context);

    SceneSnapshot Capture() const;
    void Restore(const SceneSnapshot &snapshot, int selectIndex = -2);

    void BeginEdit();
    void EndFrame(bool interactionActive);
    void RefreshStable();
    void Clear();

    bool CanUndo() const { return !m_Undo.empty(); }
    bool CanRedo() const { return !m_Redo.empty(); }
    void Undo();
    void Redo();

private:
    void Commit();

    EditorContext *m_Context;
    std::vector<SceneSnapshot> m_Undo;
    std::vector<SceneSnapshot> m_Redo;
    SceneSnapshot m_Stable;
    bool m_EditOpen = false;
    bool m_StableValid = false;

    static constexpr size_t MaxSteps = 100;
};
