#pragma once

#include <EditorPanel.h>
#include <ScriptEditor/CodeEditor.h>
#include <ScriptEditor/CodeTheme.h>
#include <ScriptEditor/LuaCompletion.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

struct EditorContext;
struct lua_State;
class AssetPickerPanel;

class ScriptEditorPanel : public EditorPanel
{
public:
    static constexpr const char *WindowId = "###ScriptEditor";

    explicit ScriptEditorPanel(EditorContext *context);
    ~ScriptEditorPanel() override;

    void OnImGuiRender() override;

    void SetAssetPicker(AssetPickerPanel *picker) { m_AssetPicker = picker; }
    void Open(const std::string &vpath);

    bool IsVisible() const { return m_Visible; }
    bool *VisibleFlag() { return &m_Visible; }
    bool HasUnsavedChanges() const;
    void FocusEditor();
    bool WantsKeyboard() const { return m_Visible && m_EditorFocused; }

private:
    struct Document
    {
        std::string VPath;
        std::string RealPath;
        std::string Name;
        ScriptEditor::CodeEditor Editor;
        uint64_t SavedVersion = 0;
        uint64_t CheckedVersion = UINT64_MAX;
        uint64_t LastEditVersion = 0;
        double LastEdit = 0.0;
        std::filesystem::file_time_type DiskTime{};
        int ErrorLine = -1;
        std::string ErrorMessage;

        bool Dirty() const { return Editor.Version() != SavedVersion; }
    };

    void EnsureInitialized();
    void DrawToolbar();
    void DrawThemeMenu();
    void DrawTabs();
    void DrawStatusBar(Document *doc);
    void DrawNewScriptPopup();
    void DrawClosePopup();

    bool Save(Document &doc);
    void Close(size_t index);
    void CheckSyntax(Document &doc);
    void ReloadFromDisk(Document &doc);
    void ApplyTheme(const std::string &name);
    std::string CreateScript(const std::string &name);
    Document *Active();

    EditorContext *m_Context = nullptr;
    AssetPickerPanel *m_AssetPicker = nullptr;

    bool m_Visible = false;
    bool m_Initialized = false;
    bool m_FocusWindow = false;
    bool m_EditorFocused = false;

    std::vector<std::unique_ptr<Document>> m_Documents;
    int m_Active = -1;
    int m_SelectTab = -1;
    int m_CloseRequest = -1;

    ScriptEditor::ThemeLibrary m_Themes;
    ScriptEditor::LuaCompletion m_Completion;
    lua_State *m_Checker = nullptr;

    char m_ThemeFilter[64] = "";
    bool m_ShowNewScript = false;
    char m_NewScriptName[64] = "";
    double m_LastDiskCheck = 0.0;
    std::string m_Status;
    double m_StatusTime = 0.0;
};
