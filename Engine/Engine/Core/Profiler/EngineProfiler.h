// EngineProfiler.h
// Drop-in CPU frame/scope profiler for a C++ game engine.
//
// Features:
//   - RAII scope timing (zero overhead when disabled)
//   - Nested call tree via scope depth
//   - Rolling frame time / FPS history (configurable window)
//   - Per-scope statistics: min / max / avg / total / call count
//   - Per-scope rolling call-count-per-frame graph
//   - ImGui viewer with:
//       * Frame time line graph
//       * FPS line graph
//       * Per-scope avg-ms bar chart (top N hotspots)
//       * Per-scope calls/frame bar chart
//       * Sortable stats table
//       * Collapsible current-frame call tree
//   - Thread-safe (single mutex, low contention path via thread_local)
//   - C++17, header-only
//
// Usage:
//   1. #include "EngineProfiler.h" everywhere you need timing macros.
//   2. Optionally #define ENGINE_PROFILER_ENABLE_IMGUI before the include
//      (or set it project-wide) to compile the ImGui panel.
//   3. In your main loop:
//
//        PROFILE_FRAME_BEGIN();
//        // ... your frame work ...
//        PROFILE_FRAME_END();
//
//   4. Inside any function or block:
//
//        void MySystem::Update()
//        {
//            PROFILE_FUNCTION();      // uses __FUNCTION__ as the label
//            // ...
//            {
//                PROFILE_SCOPE("Physics::BroadPhase");
//                // ...
//            }
//        }
//
//   5. Render the ImGui window once per frame (after EndFrame):
//
//        #ifdef ENGINE_PROFILER_ENABLE_IMGUI
//            EngineProfiler::Profiler::Get().DrawImGui();
//        #endif
//
//   6. Optional configuration (call once at startup):
//
//        EngineProfiler::Config cfg;
//        cfg.MaxFrames         = 300;   // rolling history length
//        cfg.TopScopeCount     = 10;    // hotspot bar chart entries
//        cfg.CaptureStats      = true;
//        EngineProfiler::Profiler::Get().SetConfig(cfg);
//
// Disable ALL profiling (zero overhead) by defining ENGINE_PROFILER_DISABLED:
//   #define ENGINE_PROFILER_DISABLED
//
// Requirements:
//   - C++17 or newer
//   - ImGui only if ENGINE_PROFILER_ENABLE_IMGUI is defined

#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <limits>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef ENGINE_PROFILER_ENABLE_IMGUI
// Adjust this path to wherever imgui.h lives in your project.
#include "imgui.h"
#endif

namespace EngineProfiler
{
    // -------------------------------------------------------------------------
    // Time helpers
    // -------------------------------------------------------------------------
    using Clock      = std::chrono::steady_clock;
    using Nanoseconds = std::chrono::nanoseconds;

    inline uint64_t NowNs()
    {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<Nanoseconds>(Clock::now().time_since_epoch()).count());
    }

    inline double NsToMs(uint64_t ns)  { return static_cast<double>(ns) / 1'000'000.0; }
    inline double MsToFps(double ms)   { return (ms > 0.0) ? (1000.0 / ms) : 0.0; }

    // -------------------------------------------------------------------------
    // Data structures
    // -------------------------------------------------------------------------

    struct ScopeEvent
    {
        std::string  Name;
        uint64_t     StartNs    = 0;
        uint64_t     EndNs      = 0;
        uint32_t     Depth      = 0;
        uint32_t     ThreadHash = 0;
        uint64_t     FrameIndex = 0;

        uint64_t DurationNs() const { return (EndNs >= StartNs) ? (EndNs - StartNs) : 0; }
        double   DurationMs() const { return NsToMs(DurationNs()); }
    };

    // Lifetime (all-time) statistics per named scope.
    struct ScopeStats
    {
        uint64_t Calls   = 0;
        uint64_t TotalNs = 0;
        uint64_t MinNs   = std::numeric_limits<uint64_t>::max();
        uint64_t MaxNs   = 0;

        // Rolling per-frame data (calls and avg-ms per frame, for graphs).
        // Size is capped to Config::MaxFrames by the profiler.
        std::deque<float> FrameCallCounts;   // calls this frame
        std::deque<float> FrameAvgMs;        // avg ms this frame

        void AddSample(uint64_t durationNs)
        {
            ++Calls;
            TotalNs += durationNs;
            MinNs    = std::min(MinNs, durationNs);
            MaxNs    = std::max(MaxNs, durationNs);
        }

        double AvgMs()   const { return Calls ? NsToMs(TotalNs / Calls) : 0.0; }
        double TotalMs() const { return NsToMs(TotalNs); }
        double MinMs()   const { return (MinNs == std::numeric_limits<uint64_t>::max()) ? 0.0 : NsToMs(MinNs); }
        double MaxMs()   const { return NsToMs(MaxNs); }
    };

    struct FrameStats
    {
        uint64_t FrameIndex  = 0;
        uint64_t FrameTimeNs = 0;
        double   Fps         = 0.0;
        uint32_t ScopeCount  = 0;
    };

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------
    struct Config
    {
        // How many frames of rolling history to keep for graphs.
        size_t MaxFrames     = 300;
        // How many top-hotspot scopes to show in the bar charts.
        size_t TopScopeCount = 10;
        // Whether to accumulate per-scope statistics.
        bool   CaptureStats  = true;
    };

    // -------------------------------------------------------------------------
    // Profiler singleton
    // -------------------------------------------------------------------------
    class Profiler
    {
    public:
        static Profiler &Get()
        {
            static Profiler s_Instance;
            return s_Instance;
        }

        // ------------------------------------------------------------------
        // Configuration
        // ------------------------------------------------------------------
        void SetConfig(const Config &cfg)
        {
            std::scoped_lock lock(m_Mutex);
            m_Config = cfg;
            if (m_Config.MaxFrames == 0) m_Config.MaxFrames = 1;
            TrimHistory();
        }

        const Config &GetConfig() const { return m_Config; }

        // ------------------------------------------------------------------
        // Frame control  (call once per game loop iteration)
        // ------------------------------------------------------------------
        void BeginFrame()
        {
            std::scoped_lock lock(m_Mutex);
            m_CurrentFrameEvents.clear();
            m_CurrentFrameIndex = ++m_FrameCounter;
            m_FrameStartNs      = NowNs();

            // Reset main-thread RAII stack depth.
            auto &ts   = GetThreadState();
            ts.Depth   = 0;
            ts.Stack.clear();
        }

        void EndFrame()
        {
            std::scoped_lock lock(m_Mutex);

            const uint64_t endNs       = NowNs();
            const uint64_t frameTimeNs = (endNs >= m_FrameStartNs) ? (endNs - m_FrameStartNs) : 0;

            FrameStats fs;
            fs.FrameIndex  = m_CurrentFrameIndex;
            fs.FrameTimeNs = frameTimeNs;
            fs.Fps         = MsToFps(NsToMs(frameTimeNs));
            fs.ScopeCount  = static_cast<uint32_t>(m_CurrentFrameEvents.size());

            m_FrameHistory.push_back(fs);
            if (m_FrameHistory.size() > m_Config.MaxFrames)
                m_FrameHistory.pop_front();

            if (m_Config.CaptureStats)
            {
                // Accumulate per-scope stats and per-frame data.
                // First pass: tally calls + duration per scope this frame.
                struct PerFrameAccum { uint64_t totalNs = 0; uint64_t calls = 0; };
                std::unordered_map<std::string, PerFrameAccum> frameAccum;

                for (const auto &e : m_CurrentFrameEvents)
                {
                    m_ScopeStats[e.Name].AddSample(e.DurationNs());
                    auto &a = frameAccum[e.Name];
                    a.totalNs += e.DurationNs();
                    ++a.calls;
                }

                // Push rolling per-frame series into each ScopeStats.
                for (auto &[name, stats] : m_ScopeStats)
                {
                    auto it = frameAccum.find(name);
                    float calls = 0.f, avgMs = 0.f;
                    if (it != frameAccum.end() && it->second.calls > 0)
                    {
                        calls = static_cast<float>(it->second.calls);
                        avgMs = static_cast<float>(NsToMs(it->second.totalNs / it->second.calls));
                    }
                    stats.FrameCallCounts.push_back(calls);
                    stats.FrameAvgMs.push_back(avgMs);

                    while (stats.FrameCallCounts.size() > m_Config.MaxFrames)
                        stats.FrameCallCounts.pop_front();
                    while (stats.FrameAvgMs.size() > m_Config.MaxFrames)
                        stats.FrameAvgMs.pop_front();
                }
            }
        }

        // ------------------------------------------------------------------
        // Scope push / pop  (called by ScopeTimer RAII)
        // ------------------------------------------------------------------
        void PushScope(const char *name)
        {
            if (!name) name = "(null)";
            std::scoped_lock lock(m_Mutex);
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
            std::scoped_lock lock(m_Mutex);
            auto &ts = GetThreadState();
            if (ts.Stack.empty()) return;

            const size_t idx = ts.Stack.back();
            ts.Stack.pop_back();
            if (ts.Depth > 0) --ts.Depth;

            if (idx < m_CurrentFrameEvents.size())
                m_CurrentFrameEvents[idx].EndNs = NowNs();
        }

        // ------------------------------------------------------------------
        // Read-only accessors
        // ------------------------------------------------------------------
        const std::vector<ScopeEvent>                        &GetCurrentFrameEvents() const { return m_CurrentFrameEvents; }
        const std::deque<FrameStats>                         &GetFrameHistory()        const { return m_FrameHistory; }
        const std::unordered_map<std::string, ScopeStats>   &GetScopeStats()          const { return m_ScopeStats; }

        double GetCurrentFrameTimeMs() const { return m_FrameHistory.empty() ? 0.0 : NsToMs(m_FrameHistory.back().FrameTimeNs); }
        double GetCurrentFPS()         const { return m_FrameHistory.empty() ? 0.0 : m_FrameHistory.back().Fps; }

        // Resets accumulated lifetime stats (rolling frame history is kept).
        void ResetStats()
        {
            std::scoped_lock lock(m_Mutex);
            for (auto &[name, s] : m_ScopeStats)
            {
                s.Calls   = 0;
                s.TotalNs = 0;
                s.MinNs   = std::numeric_limits<uint64_t>::max();
                s.MaxNs   = 0;
            }
        }

        // ------------------------------------------------------------------
        // ImGui panel
        // ------------------------------------------------------------------
#ifdef ENGINE_PROFILER_ENABLE_IMGUI
        // Call once per frame, after EndFrame(), while ImGui is accepting draw calls.
        // 'p_open' can be nullptr or a pointer to a bool for a close button.
        void DrawImGui(const char *title = "Profiler", bool *p_open = nullptr)
        {
            std::scoped_lock lock(m_Mutex);

            ImGui::SetNextWindowSize(ImVec2(700, 650), ImGuiCond_FirstUseEver);
            if (!ImGui::Begin(title, p_open))
            {
                ImGui::End();
                return;
            }

            // ---- Header row ------------------------------------------------
            ImGui::Text("Frame %llu", static_cast<unsigned long long>(m_CurrentFrameIndex));
            ImGui::SameLine(160); ImGui::Text("FPS: %.1f", GetCurrentFPS());
            ImGui::SameLine(280); ImGui::Text("Frame: %.3f ms", GetCurrentFrameTimeMs());
            ImGui::SameLine(440); ImGui::Text("Scopes: %u", static_cast<unsigned>(m_CurrentFrameEvents.size()));
            ImGui::SameLine();
            if (ImGui::SmallButton("Reset Stats")) { ResetStats_Locked(); }

            ImGui::Separator();

            // ---- Tabs ------------------------------------------------------
            if (ImGui::BeginTabBar("ProfilerTabs"))
            {
                if (ImGui::BeginTabItem("Graphs"))
                {
                    DrawGraphsTab();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Stats Table"))
                {
                    DrawStatsTable();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Call Tree"))
                {
                    DrawCallTree();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            ImGui::End();
        }
#endif // ENGINE_PROFILER_ENABLE_IMGUI

    private:
        // ---- Internal tree node for call-tree rendering --------------------
        struct Node
        {
            const ScopeEvent *Event = nullptr;
            std::vector<Node> Children;
        };

        // ---- Thread-local push/pop state -----------------------------------
        struct ThreadState
        {
            std::vector<size_t> Stack;
            uint32_t            Depth = 0;
        };

        static ThreadState &GetThreadState()
        {
            thread_local ThreadState s_State;
            return s_State;
        }

        static uint32_t ThreadHash()
        {
            const auto tid = std::this_thread::get_id();
            return static_cast<uint32_t>(std::hash<std::thread::id>{}(tid) & 0xFFFFFFFFu);
        }

        void TrimHistory()
        {
            while (m_FrameHistory.size() > m_Config.MaxFrames)
                m_FrameHistory.pop_front();
        }

        // Called with lock already held (from the Reset button in ImGui).
        void ResetStats_Locked()
        {
            for (auto &[name, s] : m_ScopeStats)
            {
                s.Calls   = 0;
                s.TotalNs = 0;
                s.MinNs   = std::numeric_limits<uint64_t>::max();
                s.MaxNs   = 0;
            }
        }

        // ---- ImGui helpers -------------------------------------------------
#ifdef ENGINE_PROFILER_ENABLE_IMGUI

        // Utility: convert a deque to a flat float vector for ImGui::PlotLines/PlotHistogram.
        static std::vector<float> ToVec(const std::deque<float> &d)
        {
            return std::vector<float>(d.begin(), d.end());
        }

        static std::vector<float> FrameTimesMs(const std::deque<FrameStats> &history)
        {
            std::vector<float> v;
            v.reserve(history.size());
            for (const auto &f : history)
                v.push_back(static_cast<float>(NsToMs(f.FrameTimeNs)));
            return v;
        }

        static std::vector<float> FpsValues(const std::deque<FrameStats> &history)
        {
            std::vector<float> v;
            v.reserve(history.size());
            for (const auto &f : history)
                v.push_back(static_cast<float>(f.Fps));
            return v;
        }

        // ----------------------------------------------------------------
        // Graphs Tab
        // ----------------------------------------------------------------
        void DrawGraphsTab()
        {
            const float graphW = 0.0f; // 0 = full available width
            const float graphH = 70.0f;

            // ---- Frame time ------------------------------------------------
            ImGui::TextDisabled("Frame Time (ms)");
            {
                auto data = FrameTimesMs(m_FrameHistory);
                if (!data.empty())
                {
                    float latest = data.back();
                    char overlay[32];
                    std::snprintf(overlay, sizeof(overlay), "%.2f ms", latest);
                    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.4f, 0.8f, 0.4f, 1.f));
                    ImGui::PlotLines("##FrameTime", data.data(), static_cast<int>(data.size()),
                                     0, overlay, 0.0f, 50.0f, ImVec2(graphW, graphH));
                    ImGui::PopStyleColor();
                }
                else { ImGui::TextDisabled("  (no data yet)"); }
            }

            ImGui::Spacing();

            // ---- FPS -------------------------------------------------------
            ImGui::TextDisabled("FPS");
            {
                auto data = FpsValues(m_FrameHistory);
                if (!data.empty())
                {
                    float latest = data.back();
                    char overlay[32];
                    std::snprintf(overlay, sizeof(overlay), "%.1f fps", latest);
                    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.4f, 0.6f, 1.0f, 1.f));
                    ImGui::PlotLines("##FPS", data.data(), static_cast<int>(data.size()),
                                     0, overlay, 0.0f, 300.0f, ImVec2(graphW, graphH));
                    ImGui::PopStyleColor();
                }
                else { ImGui::TextDisabled("  (no data yet)"); }
            }

            ImGui::Separator();

            // ---- Top-N hotspots: avg ms per frame --------------------------
            // Collect the top N scopes by lifetime average ms.
            struct Entry { std::string name; const ScopeStats *stats; };
            std::vector<Entry> entries;
            entries.reserve(m_ScopeStats.size());
            for (const auto &[name, s] : m_ScopeStats)
                if (s.Calls > 0) entries.push_back({ name, &s });

            const size_t topN = std::min(m_Config.TopScopeCount, entries.size());

            // Sort by lifetime avg ms, descending.
            std::partial_sort(entries.begin(), entries.begin() + static_cast<ptrdiff_t>(topN),
                              entries.end(),
                              [](const Entry &a, const Entry &b) { return a.stats->AvgMs() > b.stats->AvgMs(); });

            if (!entries.empty())
            {
                ImGui::TextDisabled("Top %zu scopes — avg ms / frame (rolling %zu frames)",
                                    topN, m_Config.MaxFrames);
                for (size_t i = 0; i < topN; ++i)
                {
                    const auto &e = entries[i];
                    auto data = ToVec(e.stats->FrameAvgMs);
                    if (data.empty()) continue;

                    char label[128], overlay[64];
                    std::snprintf(label,   sizeof(label),   "##avgms_%zu", i);
                    std::snprintf(overlay, sizeof(overlay), "%.3f ms", data.back());

                    // Colourize: red for the hottest scope, fading toward yellow.
                    float t = (topN > 1) ? (1.0f - static_cast<float>(i) / static_cast<float>(topN - 1)) : 1.0f;
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.9f * (1.f - t * 0.7f), 0.1f, 1.f));

                    // Clamp y-axis to 1.5x the first (worst) scope's peak to avoid flat lines.
                    float yMax = *std::max_element(data.begin(), data.end()) * 1.5f;
                    yMax = std::max(yMax, 0.1f);

                    // Scope name as a left-side label (truncated).
                    char nameLabel[48];
                    std::snprintf(nameLabel, sizeof(nameLabel), "%-20.20s", e.name.c_str());
                    ImGui::TextUnformatted(nameLabel);
                    ImGui::SameLine(180);
                    ImGui::PlotHistogram(label, data.data(), static_cast<int>(data.size()),
                                         0, overlay, 0.0f, yMax, ImVec2(graphW - 180, 28.0f));
                    ImGui::PopStyleColor();
                }
            }

            ImGui::Separator();

            // ---- Top-N: calls / frame -------------------------------------
            if (!entries.empty())
            {
                // Re-sort by total call count, descending.
                std::partial_sort(entries.begin(), entries.begin() + static_cast<ptrdiff_t>(topN),
                                  entries.end(),
                                  [](const Entry &a, const Entry &b) { return a.stats->Calls > b.stats->Calls; });

                ImGui::TextDisabled("Top %zu scopes — calls / frame (rolling)", topN);
                for (size_t i = 0; i < topN; ++i)
                {
                    const auto &e = entries[i];
                    auto data = ToVec(e.stats->FrameCallCounts);
                    if (data.empty()) continue;

                    char label[128], overlay[32];
                    std::snprintf(label,   sizeof(label),   "##calls_%zu", i);
                    std::snprintf(overlay, sizeof(overlay), "%.0f/frame", data.back());

                    float yMax = *std::max_element(data.begin(), data.end()) * 1.5f;
                    yMax = std::max(yMax, 1.0f);

                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.3f, 0.7f, 0.9f, 1.f));
                    char nameLabel[48];
                    std::snprintf(nameLabel, sizeof(nameLabel), "%-20.20s", e.name.c_str());
                    ImGui::TextUnformatted(nameLabel);
                    ImGui::SameLine(180);
                    ImGui::PlotHistogram(label, data.data(), static_cast<int>(data.size()),
                                          0, overlay, 0.0f, yMax, ImVec2(graphW - 180, 28.0f));
                    ImGui::PopStyleColor();
                }
            }
        }

        // ----------------------------------------------------------------
        // Stats Table Tab
        // ----------------------------------------------------------------

        // Sort column IDs.
        enum class SortCol { Name, Calls, Avg, Min, Max, Total };

        void DrawStatsTable()
        {
            // Persist sort state across frames.
            static SortCol  s_SortCol  = SortCol::Avg;
            static bool     s_SortDesc = true;

            // Filter box.
            static char s_Filter[128] = {};
            ImGui::SetNextItemWidth(200);
            ImGui::InputText("Filter##scope", s_Filter, sizeof(s_Filter));
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear##filter")) s_Filter[0] = '\0';
            ImGui::SameLine();
            ImGui::TextDisabled("%zu scopes", m_ScopeStats.size());

            const ImGuiTableFlags tableFlags =
                ImGuiTableFlags_RowBg          |
                ImGuiTableFlags_Borders        |
                ImGuiTableFlags_Resizable      |
                ImGuiTableFlags_SizingStretchProp |
                ImGuiTableFlags_ScrollY;

            const float tableH = ImGui::GetContentRegionAvail().y - 4.0f;

            if (!ImGui::BeginTable("scope_stats", 6, tableFlags, ImVec2(0, tableH)))
                return;

            // Helper lambda for sortable column headers.
            auto HeaderCol = [&](const char *label, SortCol col)
            {
                ImGui::TableSetupColumn(label, ImGuiTableColumnFlags_None);
                // We draw a custom header row below.
                (void)col;
            };

            HeaderCol("Name",      SortCol::Name);
            HeaderCol("Calls",     SortCol::Calls);
            HeaderCol("Avg ms",    SortCol::Avg);
            HeaderCol("Min ms",    SortCol::Min);
            HeaderCol("Max ms",    SortCol::Max);
            HeaderCol("Total ms",  SortCol::Total);
            ImGui::TableHeadersRow();

            // Build sorted list.
            struct Row { std::string name; const ScopeStats *s; };
            std::vector<Row> rows;
            rows.reserve(m_ScopeStats.size());
            for (const auto &[name, s] : m_ScopeStats)
            {
                if (s_Filter[0] != '\0' && name.find(s_Filter) == std::string::npos)
                    continue;
                rows.push_back({ name, &s });
            }

            std::sort(rows.begin(), rows.end(), [&](const Row &a, const Row &b)
            {
                bool less = false;
                switch (s_SortCol)
                {
                    case SortCol::Name:  less = a.name       <  b.name;           break;
                    case SortCol::Calls: less = a.s->Calls   <  b.s->Calls;       break;
                    case SortCol::Avg:   less = a.s->AvgMs()  < b.s->AvgMs();     break;
                    case SortCol::Min:   less = a.s->MinMs()  < b.s->MinMs();     break;
                    case SortCol::Max:   less = a.s->MaxMs()  < b.s->MaxMs();     break;
                    case SortCol::Total: less = a.s->TotalMs()< b.s->TotalMs();   break;
                }
                return s_SortDesc ? !less : less;
            });

            // Render rows.
            for (const auto &row : rows)
            {
                const ScopeStats &s = *row.s;

                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::TextUnformatted(row.name.c_str());

                // Colour the avg-ms cell: green → yellow → red by relative cost.
                ImGui::TableNextColumn();
                ImGui::Text("%llu", static_cast<unsigned long long>(s.Calls));

                // Colour avg cell by magnitude (rough thresholds: <1ms=ok, 1-5ms=warn, >5ms=hot).
                double avg = s.AvgMs();
                ImVec4 col = (avg < 1.0) ? ImVec4(0.5f, 1.f, 0.5f, 1.f)
                           : (avg < 5.0) ? ImVec4(1.f,  0.8f, 0.2f, 1.f)
                                         : ImVec4(1.f,  0.3f, 0.3f, 1.f);
                ImGui::TableNextColumn();
                ImGui::TextColored(col, "%.4f", avg);

                ImGui::TableNextColumn(); ImGui::Text("%.4f", s.MinMs());
                ImGui::TableNextColumn(); ImGui::Text("%.4f", s.MaxMs());
                ImGui::TableNextColumn(); ImGui::Text("%.2f",  s.TotalMs());
            }

            ImGui::EndTable();

            // Click column header to sort (manual since we're not using ImGuiTableFlags_Sortable
            // to keep compat with older ImGui versions).
            ImGui::TextDisabled("Sort: click column header row above is not interactive in this build.\n"
                                "Use the sort buttons below:");
            ImGui::SameLine();
            if (ImGui::SmallButton("Name"))  { s_SortCol = SortCol::Name;  s_SortDesc = !s_SortDesc; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Calls")) { s_SortCol = SortCol::Calls; s_SortDesc = !s_SortDesc; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Avg"))   { s_SortCol = SortCol::Avg;   s_SortDesc = !s_SortDesc; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Min"))   { s_SortCol = SortCol::Min;   s_SortDesc = !s_SortDesc; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Max"))   { s_SortCol = SortCol::Max;   s_SortDesc = !s_SortDesc; }
            ImGui::SameLine();
            if (ImGui::SmallButton("Total")) { s_SortCol = SortCol::Total; s_SortDesc = !s_SortDesc; }
            ImGui::SameLine();
            ImGui::TextDisabled(s_SortDesc ? "(desc)" : "(asc)");
        }

        // ----------------------------------------------------------------
        // Call Tree Tab
        // ----------------------------------------------------------------
        void DrawCallTree()
        {
            ImGui::TextDisabled("Current frame call tree  (%zu events)", m_CurrentFrameEvents.size());
            ImGui::Separator();

            // Rebuild tree from depth-stamped events.
            std::vector<Node>  roots;
            std::vector<Node *> stack;

            for (const auto &e : m_CurrentFrameEvents)
            {
                Node node;
                node.Event = &e;

                while (stack.size() > e.Depth)
                    stack.pop_back();

                if (stack.empty())
                {
                    roots.push_back(std::move(node));
                    stack.push_back(&roots.back());
                }
                else
                {
                    stack.back()->Children.push_back(std::move(node));
                    stack.push_back(&stack.back()->Children.back());
                }
            }

            for (auto &root : roots)
                DrawNode(root, 0);
        }

        void DrawNode(const Node &node, int depth)
        {
            if (!node.Event) return;
            const ScopeEvent &e = *node.Event;
            const double ms = e.DurationMs();

            // Colour: hot scopes glow red, fast scopes stay white.
            ImVec4 col = (ms > 5.0)  ? ImVec4(1.f, 0.3f, 0.3f, 1.f)
                       : (ms > 1.0)  ? ImVec4(1.f, 0.8f, 0.3f, 1.f)
                                     : ImGui::GetStyleColorVec4(ImGuiCol_Text);

            char label[256];
            std::snprintf(label, sizeof(label), "%s  [%.4f ms]##node_%p", e.Name.c_str(), ms, static_cast<const void*>(&e));

            if (node.Children.empty())
            {
                // Leaf node — bullet, no expand.
                ImGui::Indent(static_cast<float>(depth) * 4.0f);
                ImGui::TextColored(col, "• %s  %.4f ms", e.Name.c_str(), ms);
                ImGui::Unindent(static_cast<float>(depth) * 4.0f);
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, col);
                const bool open = ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::PopStyleColor();
                if (open)
                {
                    for (const auto &child : node.Children)
                        DrawNode(child, depth + 1);
                    ImGui::TreePop();
                }
            }
        }

#endif // ENGINE_PROFILER_ENABLE_IMGUI

        // ---- Members -------------------------------------------------------
        Config    m_Config{};
        std::mutex m_Mutex;

        uint64_t m_FrameCounter       = 0;
        uint64_t m_CurrentFrameIndex  = 0;
        uint64_t m_FrameStartNs       = 0;

        std::vector<ScopeEvent>                       m_CurrentFrameEvents;
        std::deque<FrameStats>                        m_FrameHistory;
        std::unordered_map<std::string, ScopeStats>  m_ScopeStats;

        Profiler() = default;
    };

    // -------------------------------------------------------------------------
    // RAII scope timer
    // -------------------------------------------------------------------------
    class ScopeTimer
    {
    public:
        explicit ScopeTimer(const char *name) : m_Name(name)
        {
            Profiler::Get().PushScope(m_Name);
        }
        ~ScopeTimer() { Profiler::Get().PopScope(); }

        ScopeTimer(const ScopeTimer &)             = delete;
        ScopeTimer &operator=(const ScopeTimer &)  = delete;

    private:
        const char *m_Name;
    };

} // namespace EngineProfiler

// =============================================================================
// Macros
// =============================================================================
//
// ENGINE_PROFILER_DISABLED disables all macros (zero overhead).
//
#ifndef ENGINE_PROFILER_DISABLED
    #define PROFILE_FRAME_BEGIN()   ::EngineProfiler::Profiler::Get().BeginFrame()
    #define PROFILE_FRAME_END()     ::EngineProfiler::Profiler::Get().EndFrame()
    #define PROFILE_SCOPE(name)     ::EngineProfiler::ScopeTimer _profScope_##__LINE__(name)
    #define PROFILE_FUNCTION()      PROFILE_SCOPE(__FUNCTION__)
#else
    #define PROFILE_FRAME_BEGIN()   do {} while(0)
    #define PROFILE_FRAME_END()     do {} while(0)
    #define PROFILE_SCOPE(name)     do {} while(0)
    #define PROFILE_FUNCTION()      do {} while(0)
#endif