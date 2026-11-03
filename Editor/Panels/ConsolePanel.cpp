#include "ConsolePanel.h"
#include "UI/Widgets.h"

#include <imgui_internal.h>

#include <Engine/Core/Log.h>
#include <Engine/ImGuiSink.h>

#include <cstdio>
#include <vector>

namespace
{
    const ImVec4 &LevelColor(spdlog::level::level_enum level)
    {
        using namespace EditorUI;
        if (level <= spdlog::level::debug)
            return Colors::TextFaint;
        if (level == spdlog::level::warn)
            return Colors::Warning;
        if (level >= spdlog::level::err)
            return Colors::Error;
        return Colors::Text;
    }

    void FilterToggle(const char *label, int count, bool *value)
    {
        using namespace EditorUI;

        char text[48];
        std::snprintf(text, sizeof(text), "%s %d###%s", label, count, label);
        ImGui::PushStyleColor(ImGuiCol_Text, *value ? Colors::Text : Colors::TextFaint);
        if (Button(text, *value ? ButtonKind::Secondary : ButtonKind::Ghost))
            *value = !*value;
        ImGui::PopStyleColor();
    }
}

void ConsolePanel::OnImGuiRender()
{
    using namespace EditorUI;

    if (!m_Visible)
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    bool visible = ImGui::Begin("Console###Console", &m_Visible);
    ImGui::PopStyleVar();
    if (!visible)
    {
        ImGui::End();
        return;
    }

    auto sink = SIMPEngine::Log::GetImGuiSink();
    if (!sink)
    {
        EmptyState("No log sink");
        ImGui::End();
        return;
    }

    int counts[4] = {0, 0, 0, 0};
    std::vector<int> rows;

    sink->ForEach([&](const std::vector<SIMPEngine::LogEntry> &entries)
                  {
        for (int index = 0; index < static_cast<int>(entries.size()); ++index)
        {
            const auto &e = entries[index];
            int bucket = 1;
            bool show = m_ShowInfo;
            if (e.Level <= spdlog::level::debug)
            {
                bucket = 0;
                show = m_ShowTrace;
            }
            else if (e.Level == spdlog::level::warn)
            {
                bucket = 2;
                show = m_ShowWarn;
            }
            else if (e.Level >= spdlog::level::err)
            {
                bucket = 3;
                show = m_ShowError;
            }
            ++counts[bucket];

            if (show && ContainsInsensitive(e.Message.c_str(), m_Filter))
                rows.push_back(index);
        } });

    BeginToolbar("##console_toolbar");
    if (Button("Clear", ButtonKind::Ghost))
    {
        rows.clear();
        sink->Clear();
        counts[0] = counts[1] = counts[2] = counts[3] = 0;
    }
    ImGui::SameLine(0, 4);
    SearchBox("##filter", m_Filter, sizeof(m_Filter), "Filter", 220.0f);
    ImGui::SameLine(0, 8);
    FilterToggle("Debug", counts[0], &m_ShowTrace);
    ImGui::SameLine();
    FilterToggle("Info", counts[1], &m_ShowInfo);
    ImGui::SameLine();
    FilterToggle("Warnings", counts[2], &m_ShowWarn);
    ImGui::SameLine();
    FilterToggle("Errors", counts[3], &m_ShowError);
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize("Auto-scroll").x - ImGui::GetFrameHeight() - 8.0f);
    ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
    EndToolbar();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Frame);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 4));
    ImGui::BeginChild("##log", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PopStyleVar();

    if (rows.empty())
    {
        EmptyState(m_Filter[0] ? "No messages match the filter" : "No messages");
    }
    else
    {
        ImGui::PushFont(GetFont(FontRole::Mono), 0.0f);
        sink->ForEach([&](const std::vector<SIMPEngine::LogEntry> &entries)
                      {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 0));
            float rowH = ImGui::GetTextLineHeight() + 3.0f;
            float timeW = ImGui::CalcTextSize("00:00:00").x;
            float loggerW = ImGui::CalcTextSize("ENGINE").x;

            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(rows.size()), rowH);
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                {
                    if (rows[i] >= static_cast<int>(entries.size()))
                        continue;
                    const SIMPEngine::LogEntry &e = entries[rows[i]];
                    const ImVec4 &color = LevelColor(e.Level);

                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    float width = ImGui::GetContentRegionAvail().x;
                    ImDrawList *dl = ImGui::GetWindowDrawList();

                    if (e.Level >= spdlog::level::warn)
                        dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + rowH), U32(color, 0.08f));

                    float y = pos.y + 1.5f;
                    float x = pos.x;
                    dl->AddText(ImVec2(x, y), U32(Colors::TextFaint), e.Time.c_str());
                    x += timeW + 12.0f;
                    dl->AddText(ImVec2(x, y), U32(Colors::TextDim), e.Logger.c_str());
                    x += loggerW + 12.0f;
                    dl->AddText(ImVec2(x, y), U32(color), e.Message.c_str());

                    float textW = ImGui::CalcTextSize(e.Message.c_str()).x;
                    ImGui::PushID(i);
                    ImGui::InvisibleButton("##row", ImVec2(std::max(width, x - pos.x + textW + 8.0f), rowH));
                    if (ImGui::BeginPopupContextItem("##rowctx"))
                    {
                        if (ImGui::MenuItem("Copy Message"))
                            ImGui::SetClipboardText(e.Message.c_str());
                        ImGui::EndPopup();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::PopStyleVar(); });
        ImGui::PopFont();

        if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.0f)
            ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();
}
