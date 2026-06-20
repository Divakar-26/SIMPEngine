cat > /home/divakar/Desktop/SDLGameEngine/Engine/Engine/Core/Profiler/EngineProfiler.h << 'HEADER_EOF'
// EngineProfiler.h
// Drop-in CPU frame/scope profiler for SIMPEngine.
//
// ---- USAGE (macros are identical, nothing to change) ----
//
//   PROFILE_FRAME_BEGIN();          // first line of your game loop
//   PROFILE_FRAME_END();            // last line of your game loop
//
//   PROFILE_FUNCTION();             // top of any function you want to time
//   PROFILE_SCOPE("MyLabel");       // time any arbitrary block
//
//   EngineProfiler::Profiler::Get().DrawImGui();   // in your ImGui layer
//
// ---- CMake ----
//   target_compile_definitions(Engine PUBLIC ENGINE_PROFILER_ENABLE_IMGUI)
//
// ---- Disable entirely (zero overhead) ----
//   #define ENGINE_PROFILER_DISABLED

#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef ENGINE_PROFILER_ENABLE_IMGUI
#include "imgui.h"
#endif

namespace EngineProfiler
{
    // =========================================================================
    // Internal time utilities
    // =========================================================================
    using Clock       = std::chrono::steady_clock;
    using Nanoseconds = std::chrono::nanoseconds;

    inline uint64_t NowNs()
    {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<Nanoseconds>(
                Clock::now().time_since_epoch()).count());
    }
    inline double NsToMs(uint64_t ns) { return static_cast<double>(ns) / 1'000'000.0; }
    inline double MsToFps(double ms)  { return (ms > 0.0) ? (1000.0 / ms) : 0.0; }

    // =========================================================================
    // Public data types
    // =========================================================================

    // One timed region recorded this frame.
    struct ScopeEvent
    {
        std::string Name;
        uint64_t    StartNs    = 0;
        uint64_t    EndNs      = 0;
        uint32_t    Depth      = 0;   // nesting level (0 = root)
        uint32_t    ThreadHash = 0;
        uint64_t    FrameIndex = 0;

        uint64_t DurationNs() const { return EndNs >= StartNs ? EndNs - StartNs : 0; }
        double   DurationMs() const { return NsToMs(DurationNs()); }
    };

    // Accumulated lifetime statistics for one named scope.
    struct ScopeStats
    {
        uint64_t Calls   = 0;
        uint64_t TotalNs = 0;
        uint64_t MinNs   = std::numeric_limits<uint64_t>::max();
        uint64_t MaxNs   = 0;

        // Rolling per-frame series (length == Config::MaxFrames).
        std::deque<float> FrameCallCounts;
        std::deque<float> FrameAvgMs;

        void AddSample(uint64_t ns)
        {
            ++Calls;
            TotalNs += ns;
            if (ns < MinNs) MinNs = ns;
            if (ns > MaxNs) MaxNs = ns;
        }

        double AvgMs()   const { return Calls ? NsToMs(TotalNs / Calls) : 0.0; }
        double TotalMs() const { return NsToMs(TotalNs); }
        double MinMs()   const { return MinNs == std::numeric_limits<uint64_t>::max() ? 0.0 : NsToMs(MinNs); }
        double MaxMs()   const { return NsToMs(MaxNs); }
    };

    struct FrameStats
    {
        uint64_t FrameIndex  = 0;
        uint64_t FrameTimeNs = 0;
        double   Fps         = 0.0;
        uint32_t ScopeCount  = 0;
    };

    struct Config
    {
        size_t MaxFrames     = 300;   // rolling graph window
        size_t TopScopeCount = 12;    // hotspot bar chart rows
        bool   CaptureStats  = true;
    };

    // =========================================================================
    // Snapshot — what DrawImGui works on (lock-free after copy)
    // =========================================================================
    struct Snapshot
    {
        uint64_t                                    FrameIndex  = 0;
        double                                      FrameTimeMs = 0.0;
        double                                      Fps         = 0.0;
        uint32_t                                    ScopeCount  = 0;
        std::vector<ScopeEvent>                     Events;
        std::deque<FrameStats>                      History;
        std::unordered_map<std::string, ScopeStats> Stats;
    };

    // =========================================================================
    // Profiler singleton
    // =========================================================================
    class Profiler
    {
    public:
        static Profiler &Get()
        {
            static Profiler s;
            return s;
        }

        // ---------------------------------------------------------------------
        // Config
        // ---------------------------------------------------------------------
        void SetConfig(const Config &cfg)
        {
            std::scoped_lock lk(m_Mutex);
            m_Config = cfg;
            if (!m_Config.MaxFrames) m_Config.MaxFrames = 1;
            while (m_FrameHistory.size() > m_Config.MaxFrames)
                m_FrameHistory.pop_front();
        }

        // ---------------------------------------------------------------------
        // Frame boundary  — PROFILE_FRAME_BEGIN / PROFILE_FRAME_END
        // ---------------------------------------------------------------------
        void BeginFrame()
        {
            std::scoped_lock lk(m_Mutex);
            m_CurrentFrameEvents.clear();
            m_CurrentFrameIndex = ++m_FrameCounter;
            m_FrameStartNs      = NowNs();
            auto &ts = GetThreadState();
            ts.Depth = 0;
            ts.Stack.clear();
        }

        void EndFrame()
        {
            std::scoped_lock lk(m_Mutex);

            const uint64_t frameNs = NowNs() - m_FrameStartNs;

            FrameStats fs;
            fs.FrameIndex  = m_CurrentFrameIndex;
            fs.FrameTimeNs = frameNs;
            fs.Fps         = MsToFps(NsToMs(frameNs));
            fs.ScopeCount  = static_cast<uint32_t>(m_CurrentFrameEvents.size());
            m_FrameHistory.push_back(fs);
            while (m_FrameHistory.size() > m_Config.MaxFrames)
                m_FrameHistory.pop_front();

            if (m_Config.CaptureStats)
            {
                struct Accum { uint64_t totalNs = 0; uint64_t calls = 0; };
                std::unordered_map<std::string, Accum> acc;
                for (const auto &e : m_CurrentFrameEvents)
                {
                    m_ScopeStats[e.Name].AddSample(e.DurationNs());
                    acc[e.Name].totalNs += e.DurationNs();
                    ++acc[e.Name].calls;
                }
                for (auto &[name, st] : m_ScopeStats)
                {
                    auto it = acc.find(name);
                    float calls = 0.f, avgMs = 0.f;
                    if (it != acc.end() && it->second.calls)
                    {
                        calls = static_cast<float>(it->second.calls);
                        avgMs = static_cast<float>(NsToMs(it->second.totalNs / it->second.calls));
                    }
                    st.FrameCallCounts.push_back(calls);
                    st.FrameAvgMs.push_back(avgMs);
                    while (st.FrameCallCounts.size() > m_Config.MaxFrames) st.FrameCallCounts.pop_front();
                    while (st.FrameAvgMs.size()       > m_Config.MaxFrames) st.FrameAvgMs.pop_front();
                }
            }
        }

        // ---------------------------------------------------------------------
        // Scope push/pop — called by ScopeTimer RAII
        // ---------------------------------------------------------------------
        void PushScope(const char *name)
        {
            if (!name) name = "(null)";
            std::scoped_lock lk(m_Mutex);
            auto &ts = GetThreadState();

            ScopeEvent e;
            e.Name       = name;
            e.StartNs    = NowNs();
            e.Depth      = ts.Depth;
            e.ThreadHash = ThreadHash();
            e.FrameIndex = m_CurrentFrameIndex;

            m_CurrentFrameEvents.push_back(std::move(e));
            ts.Stack.push_back(m_CurrentFrameEvents.size() - 1);
            ++ts.Depth;
        }

        void PopScope()
        {
            std::scoped_lock lk(m_Mutex);
            auto &ts = GetThreadState();
            if (ts.Stack.empty()) return;
            const size_t idx = ts.Stack.back();
            ts.Stack.pop_back();
            if (ts.Depth) --ts.Depth;
            if (idx < m_CurrentFrameEvents.size())
                m_CurrentFrameEvents[idx].EndNs = NowNs();
        }

        // ---------------------------------------------------------------------
        // Reset lifetime stats
        // ---------------------------------------------------------------------
        void ResetStats()
        {
            std::scoped_lock lk(m_Mutex);
            ResetStats_Locked();
        }

        // ---------------------------------------------------------------------
        // Take a lock-free snapshot for rendering
        // ---------------------------------------------------------------------
        Snapshot TakeSnapshot()
        {
            std::scoped_lock lk(m_Mutex);
            Snapshot s;
            s.FrameIndex  = m_CurrentFrameIndex;
            s.FrameTimeMs = m_FrameHistory.empty() ? 0.0 : NsToMs(m_FrameHistory.back().FrameTimeNs);
            s.Fps         = m_FrameHistory.empty() ? 0.0 : m_FrameHistory.back().Fps;
            s.ScopeCount  = static_cast<uint32_t>(m_CurrentFrameEvents.size());
            s.Events      = m_CurrentFrameEvents;   // copy
            s.History     = m_FrameHistory;          // copy
            s.Stats       = m_ScopeStats;            // copy
            return s;
        }

        // ---------------------------------------------------------------------
        // ImGui panel  (lock-free: snapshots data first, then renders)
        // ---------------------------------------------------------------------
#ifdef ENGINE_PROFILER_ENABLE_IMGUI
        void DrawImGui(const char *title = "Profiler", bool *p_open = nullptr)
        {
            // ---- Snapshot under lock, render without it --------------------
            Snapshot snap = TakeSnapshot();

            ImGui::SetNextWindowSize(ImVec2(780, 700), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowBgAlpha(0.92f);

            ImGuiWindowFlags wf = ImGuiWindowFlags_NoScrollbar;
            if (!ImGui::Begin(title, p_open, wf)) { ImGui::End(); return; }

            // ---- Top bar ---------------------------------------------------
            DrawTopBar(snap);
            ImGui::Separator();

            // ---- Tabs ------------------------------------------------------
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 5));
            if (ImGui::BeginTabBar("##ptabs"))
            {
                if (ImGui::BeginTabItem("  Graphs  "))
                {
                    ImGui::PopStyleVar();
                    DrawGraphsTab(snap);
                    ImGui::EndTabItem();
                }
                else if (ImGui::BeginTabItem("  Stats  "))
                {
                    ImGui::PopStyleVar();
                    DrawStatsTab(snap);
                    ImGui::EndTabItem();
                }
                else if (ImGui::BeginTabItem("  Call Tree  "))
                {
                    ImGui::PopStyleVar();
                    DrawCallTreeTab(snap);
                    ImGui::EndTabItem();
                }
                else
                {
                    ImGui::PopStyleVar();
                }
                ImGui::EndTabBar();
            }

            ImGui::End();
        }
#endif

    private:
        Profiler() = default;

        // ---- Thread-local push/pop bookkeeping -----------------------------
        struct ThreadState { std::vector<size_t> Stack; uint32_t Depth = 0; };
        static ThreadState &GetThreadState() { thread_local ThreadState s; return s; }
        static uint32_t ThreadHash()
        {
            return static_cast<uint32_t>(
                std::hash<std::thread::id>{}(std::this_thread::get_id()) & 0xFFFFFFFFu);
        }

        void ResetStats_Locked()
        {
            for (auto &[n, s] : m_ScopeStats)
            {
                s.Calls = 0; s.TotalNs = 0;
                s.MinNs = std::numeric_limits<uint64_t>::max(); s.MaxNs = 0;
            }
        }

        // =========================================================================
        // ImGui drawing helpers (all work on Snapshot, no lock needed)
        // =========================================================================
#ifdef ENGINE_PROFILER_ENABLE_IMGUI

        // ---- Colour helpers -------------------------------------------------
        static ImVec4 MsColor(double ms)
        {
            if (ms < 0.5)  return ImVec4(0.40f, 0.90f, 0.40f, 1.f); // green
            if (ms < 2.0)  return ImVec4(0.95f, 0.85f, 0.20f, 1.f); // yellow
            if (ms < 5.0)  return ImVec4(1.00f, 0.55f, 0.10f, 1.f); // orange
            return             ImVec4(1.00f, 0.25f, 0.25f, 1.f);     // red
        }
        static ImVec4 HeatColor(float t) // t in [0,1], 0=cool 1=hot
        {
            return ImVec4(0.2f + 0.8f * t, 0.8f - 0.6f * t, 0.1f, 1.f);
        }

        static std::vector<float> DequeToVec(const std::deque<float> &d)
        { return std::vector<float>(d.begin(), d.end()); }

        // ---- Top status bar ------------------------------------------------
        void DrawTopBar(const Snapshot &s)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.f));
            ImGui::Text("Frame"); ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("%llu", static_cast<unsigned long long>(s.FrameIndex));

            ImGui::SameLine(0, 24);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.f));
            ImGui::Text("FPS"); ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextColored(MsColor(1000.0 / (s.Fps > 0 ? s.Fps : 1)), "%.1f", s.Fps);

            ImGui::SameLine(0, 24);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.f));
            ImGui::Text("Frame"); ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextColored(MsColor(s.FrameTimeMs), "%.2f ms", s.FrameTimeMs);

            ImGui::SameLine(0, 24);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.f));
            ImGui::Text("Scopes"); ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("%u", s.ScopeCount);

            ImGui::SameLine(0, 24);
            if (ImGui::SmallButton("Reset Stats"))
            {
                std::scoped_lock lk(m_Mutex);
                ResetStats_Locked();
            }
        }

        // =====================================================================
        // Graphs tab
        // =====================================================================
        void DrawGraphsTab(const Snapshot &snap)
        {
            const float W = ImGui::GetContentRegionAvail().x;
            const float lineH = 60.0f;
            const float barH  = 22.0f;

            // ---- Frame time ------------------------------------------------
            {
                std::vector<float> data;
                data.reserve(snap.History.size());
                for (auto &f : snap.History)
                    data.push_back(static_cast<float>(NsToMs(f.FrameTimeNs)));

                SectionLabel("Frame Time");
                if (!data.empty())
                {
                    char ov[32]; std::snprintf(ov, sizeof(ov), "%.2f ms", data.back());
                    ImGui::PushStyleColor(ImGuiCol_PlotLines,      ImVec4(0.35f, 0.85f, 0.35f, 1.f));
                    ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered,ImVec4(0.6f,  1.0f,  0.6f,  1.f));
                    ImGui::PlotLines("##ft", data.data(), (int)data.size(), 0, ov, 0.f, 50.f, ImVec2(W, lineH));
                    ImGui::PopStyleColor(2);
                    // Threshold lines hint
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,0.5f,0.5f,1.f));
                    ImGui::Text("  16.6ms = 60fps   33.3ms = 30fps");
                    ImGui::PopStyleColor();
                }
            }

            ImGui::Spacing();

            // ---- FPS -------------------------------------------------------
            {
                std::vector<float> data;
                data.reserve(snap.History.size());
                for (auto &f : snap.History)
                    data.push_back(static_cast<float>(f.Fps));

                SectionLabel("FPS");
                if (!data.empty())
                {
                    char ov[32]; std::snprintf(ov, sizeof(ov), "%.0f", data.back());
                    ImGui::PushStyleColor(ImGuiCol_PlotLines,       ImVec4(0.35f, 0.60f, 1.0f, 1.f));
                    ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, ImVec4(0.60f, 0.80f, 1.0f, 1.f));
                    ImGui::PlotLines("##fps", data.data(), (int)data.size(), 0, ov, 0.f, 300.f, ImVec2(W, lineH));
                    ImGui::PopStyleColor(2);
                }
            }

            ImGui::Separator(); ImGui::Spacing();

            // ---- Build sorted entry list -----------------------------------
            struct Entry { std::string name; const ScopeStats *s; };
            std::vector<Entry> byMs, byCalls;
            for (const auto &[n, s] : snap.Stats)
                if (s.Calls) { byMs.push_back({n,&s}); byCalls.push_back({n,&s}); }

            const size_t topN = std::min(m_Config.TopScopeCount, byMs.size());
            if (!topN) { ImGui::TextDisabled("No scope data yet."); ImGui::End(); return; }

            std::partial_sort(byMs.begin(),    byMs.begin()    + topN, byMs.end(),
                [](const Entry &a, const Entry &b){ return a.s->AvgMs() > b.s->AvgMs(); });
            std::partial_sort(byCalls.begin(), byCalls.begin() + topN, byCalls.end(),
                [](const Entry &a, const Entry &b){ return a.s->Calls > b.s->Calls; });

            const float labelW = 160.f;
            const float barW   = W - labelW - 8.f;

            // ---- Avg ms / frame bar chart ----------------------------------
            SectionLabel("Hottest scopes — avg ms/frame");
            for (size_t i = 0; i < topN; ++i)
            {
                const auto &e = byMs[i];
                auto data = DequeToVec(e.s->FrameAvgMs);
                if (data.empty()) continue;

                float yMax = *std::max_element(data.begin(), data.end()) * 1.6f;
                yMax = std::max(yMax, 0.05f);

                float t = topN > 1 ? (1.f - (float)i / (float)(topN-1)) : 1.f;
                char lbl[64], ov[32], id[32];
                std::snprintf(lbl, sizeof(lbl), "%-22.22s", e.name.c_str());
                std::snprintf(ov,  sizeof(ov),  "%.3fms",   data.back());
                std::snprintf(id,  sizeof(id),  "##am%zu",  i);

                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, HeatColor(t));
                ImGui::TextUnformatted(lbl);
                ImGui::SameLine(labelW);
                ImGui::PlotHistogram(id, data.data(), (int)data.size(), 0, ov, 0.f, yMax, ImVec2(barW, barH));
                ImGui::PopStyleColor();
            }

            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            // ---- Calls / frame bar chart -----------------------------------
            SectionLabel("Most-called scopes — calls/frame");
            for (size_t i = 0; i < topN; ++i)
            {
                const auto &e = byCalls[i];
                auto data = DequeToVec(e.s->FrameCallCounts);
                if (data.empty()) continue;

                float yMax = *std::max_element(data.begin(), data.end()) * 1.6f;
                yMax = std::max(yMax, 1.f);

                char lbl[64], ov[32], id[32];
                std::snprintf(lbl, sizeof(lbl), "%-22.22s", e.name.c_str());
                std::snprintf(ov,  sizeof(ov),  "%.0f/f",   data.back());
                std::snprintf(id,  sizeof(id),  "##cf%zu",  i);

                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.25f, 0.65f, 0.95f, 1.f));
                ImGui::TextUnformatted(lbl);
                ImGui::SameLine(labelW);
                ImGui::PlotHistogram(id, data.data(), (int)data.size(), 0, ov, 0.f, yMax, ImVec2(barW, barH));
                ImGui::PopStyleColor();
            }
        }

        // =====================================================================
        // Stats table tab
        // =====================================================================
        void DrawStatsTab(const Snapshot &snap)
        {
            static int  s_sortCol  = 2;   // 0=Name 1=Calls 2=Avg 3=Min 4=Max 5=Total
            static bool s_sortDesc = true;

            // ---- Filter + controls -----------------------------------------
            static char s_filter[128] = {};
            ImGui::SetNextItemWidth(200);
            ImGui::InputText("Filter##sf", s_filter, sizeof(s_filter));
            ImGui::SameLine();
            if (ImGui::SmallButton("X##clf")) s_filter[0] = '\0';
            ImGui::SameLine(0, 16);
            ImGui::TextDisabled("%zu scopes total", snap.Stats.size());

            ImGui::Spacing();

            // ---- Sort buttons (compact row) --------------------------------
            ImGui::TextDisabled("Sort:");
            const char *cols[] = { "Name","Calls","Avg","Min","Max","Total" };
            for (int i = 0; i < 6; ++i)
            {
                ImGui::SameLine();
                bool active = (s_sortCol == i);
                if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f,0.5f,0.8f,1.f));
                char btnlbl[32];
                std::snprintf(btnlbl, sizeof(btnlbl), "%s%s##sb%d",
                              cols[i], (active ? (s_sortDesc ? " ▼" : " ▲") : ""), i);
                if (ImGui::SmallButton(btnlbl))
                {
                    if (s_sortCol == i) s_sortDesc = !s_sortDesc;
                    else { s_sortCol = i; s_sortDesc = true; }
                }
                if (active) ImGui::PopStyleColor();
            }

            ImGui::Spacing();

            // ---- Table -----------------------------------------------------
            const ImGuiTableFlags tf =
                ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_SizingStretchProp;

            float tableH = ImGui::GetContentRegionAvail().y - 4.f;
            if (!ImGui::BeginTable("##st", 6, tf, ImVec2(0, tableH))) return;

            ImGui::TableSetupColumn("Name",     ImGuiTableColumnFlags_WidthStretch, 3.f);
            ImGui::TableSetupColumn("Calls",    ImGuiTableColumnFlags_WidthStretch, 1.f);
            ImGui::TableSetupColumn("Avg ms",   ImGuiTableColumnFlags_WidthStretch, 1.f);
            ImGui::TableSetupColumn("Min ms",   ImGuiTableColumnFlags_WidthStretch, 1.f);
            ImGui::TableSetupColumn("Max ms",   ImGuiTableColumnFlags_WidthStretch, 1.f);
            ImGui::TableSetupColumn("Total ms", ImGuiTableColumnFlags_WidthStretch, 1.f);
            ImGui::TableHeadersRow();

            // Build & sort rows
            struct Row { std::string name; ScopeStats s; };
            std::vector<Row> rows;
            rows.reserve(snap.Stats.size());
            for (const auto &[n, st] : snap.Stats)
            {
                if (s_filter[0] && n.find(s_filter) == std::string::npos) continue;
                rows.push_back({n, st});
            }
            std::sort(rows.begin(), rows.end(), [&](const Row &a, const Row &b)
            {
                bool less = false;
                switch (s_sortCol)
                {
                    case 0: less = a.name        < b.name;         break;
                    case 1: less = a.s.Calls     < b.s.Calls;      break;
                    case 2: less = a.s.AvgMs()   < b.s.AvgMs();    break;
                    case 3: less = a.s.MinMs()   < b.s.MinMs();    break;
                    case 4: less = a.s.MaxMs()   < b.s.MaxMs();    break;
                    case 5: less = a.s.TotalMs() < b.s.TotalMs();  break;
                }
                return s_sortDesc ? !less : less;
            });

            // Draw rows
            for (const auto &row : rows)
            {
                const ScopeStats &s = row.s;
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(row.name.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%llu", (unsigned long long)s.Calls);

                double avg = s.AvgMs();
                ImGui::TableNextColumn();
                ImGui::TextColored(MsColor(avg), "%.4f", avg);

                ImGui::TableNextColumn(); ImGui::Text("%.4f", s.MinMs());

                // Max gets a heat tint too
                ImGui::TableNextColumn();
                ImGui::TextColored(MsColor(s.MaxMs()), "%.4f", s.MaxMs());

                ImGui::TableNextColumn(); ImGui::Text("%.2f", s.TotalMs());
            }

            ImGui::EndTable();
        }

        // =====================================================================
        // Call tree tab
        // =====================================================================

        struct Node { const ScopeEvent *E = nullptr; std::vector<Node> Children; };

        void DrawCallTreeTab(const Snapshot &snap)
        {
            ImGui::TextDisabled("Current frame — %zu events recorded", snap.Events.size());
            ImGui::Separator();

            if (snap.Events.empty())
            {
                ImGui::Spacing();
                ImGui::TextDisabled("  No events this frame.");
                ImGui::TextDisabled("  Make sure PROFILE_FRAME_BEGIN() is the very first");
                ImGui::TextDisabled("  call in your loop, before any PROFILE_SCOPE/FUNCTION.");
                return;
            }

            // Build tree from depth-ordered flat list
            std::vector<Node>  roots;
            std::vector<Node *> stk;

            for (const auto &e : snap.Events)
            {
                Node node; node.E = &e;
                while (stk.size() > e.Depth) stk.pop_back();
                if (stk.empty()) { roots.push_back(std::move(node)); stk.push_back(&roots.back()); }
                else { stk.back()->Children.push_back(std::move(node)); stk.push_back(&stk.back()->Children.back()); }
            }

            ImGui::BeginChild("##calltree", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
            for (const auto &r : roots) DrawNode(r);
            ImGui::EndChild();
        }

        void DrawNode(const Node &node)
        {
            if (!node.E) return;
            const ScopeEvent &e = *node.E;
            const double ms = e.DurationMs();
            ImVec4 col = MsColor(ms);

            char label[256];
            std::snprintf(label, sizeof(label), "%s  [%.4f ms]##%p",
                          e.Name.c_str(), ms, static_cast<const void*>(&e));

            if (node.Children.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, col);
                ImGui::BulletText("%s  %.4f ms", e.Name.c_str(), ms);
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, col);
                bool open = ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::PopStyleColor();
                if (open)
                {
                    for (const auto &c : node.Children) DrawNode(c);
                    ImGui::TreePop();
                }
            }
        }

        // ---- Tiny section header -------------------------------------------
        static void SectionLabel(const char *text)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.75f, 1.0f, 1.f));
            ImGui::TextUnformatted(text);
            ImGui::PopStyleColor();
        }

#endif // ENGINE_PROFILER_ENABLE_IMGUI

        // ---- Members -------------------------------------------------------
        Config     m_Config{};
        std::mutex m_Mutex;

        uint64_t m_FrameCounter      = 0;
        uint64_t m_CurrentFrameIndex = 0;
        uint64_t m_FrameStartNs      = 0;

        std::vector<ScopeEvent>                      m_CurrentFrameEvents;
        std::deque<FrameStats>                       m_FrameHistory;
        std::unordered_map<std::string, ScopeStats>  m_ScopeStats;
    };

    // =========================================================================
    // RAII scope timer
    // =========================================================================
    class ScopeTimer
    {
    public:
        explicit ScopeTimer(const char *name) { Profiler::Get().PushScope(name); }
        ~ScopeTimer()                         { Profiler::Get().PopScope(); }
        ScopeTimer(const ScopeTimer &)            = delete;
        ScopeTimer &operator=(const ScopeTimer &) = delete;
    };

} // namespace EngineProfiler

// =============================================================================
// Macros  (identical to before — nothing in your codebase needs to change)
// =============================================================================
#ifndef ENGINE_PROFILER_DISABLED
    #define PROFILE_FRAME_BEGIN()  ::EngineProfiler::Profiler::Get().BeginFrame()
    #define PROFILE_FRAME_END()    ::EngineProfiler::Profiler::Get().EndFrame()
    #define PROFILE_SCOPE(name)    ::EngineProfiler::ScopeTimer _profScope_##__LINE__(name)
    #define PROFILE_FUNCTION()     PROFILE_SCOPE(__FUNCTION__)
#else
    #define PROFILE_FRAME_BEGIN()  do {} while(0)
    #define PROFILE_FRAME_END()    do {} while(0)
    #define PROFILE_SCOPE(name)    do {} while(0)
    #define PROFILE_FUNCTION()     do {} while(0)
#endif
HEADER_EOF
echo "Done, $(wc -l < /home/divakar/Desktop/SDLGameEngine/Engine/Engine/Core/Profiler/EngineProfiler.h) lines"
