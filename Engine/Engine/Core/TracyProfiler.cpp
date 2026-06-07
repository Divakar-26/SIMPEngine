#include <Engine/Core/TracyProfiler.h>
#include <Engine/Core/Log.h>
#include <cstring>

namespace SIMPEngine::Profiling {

    FrameStats g_FrameStats;

    void InitProfiler() {
        #ifdef ENABLE_TRACY_PROFILING
            CORE_INFO("Tracy Profiler enabled - connect with Tracy client to visualize");
        #else
            CORE_INFO("Tracy Profiler disabled - define ENABLE_TRACY_PROFILING to enable");
        #endif
    }

    FrameStats GetFrameStats() {
        return g_FrameStats;
    }

    void UpdateFrameStats(float dt, int drawCalls, int entityCount) {
        g_FrameStats.frameTime = dt;
        g_FrameStats.fps = dt > 0 ? 1000.0f / dt : 0.0f;
        g_FrameStats.drawCalls = drawCalls;
        g_FrameStats.entityCount = entityCount;
        
        #ifdef ENABLE_TRACY_PROFILING
            PROFILE_PLOT("Frame Time (ms)", static_cast<int64_t>(dt));
            PROFILE_PLOT("FPS", static_cast<int64_t>(g_FrameStats.fps));
            PROFILE_PLOT("Draw Calls", drawCalls);
            PROFILE_PLOT("Entity Count", entityCount);
        #endif
    }

    void* AllocateTracked(size_t size, const char* label) {
        void* ptr = malloc(size);
        #ifdef ENABLE_TRACY_PROFILING
            if (label) {
                PROFILE_MESSAGE(label, strlen(label));
            }
            PROFILE_ALLOC(ptr, size);
        #endif
        return ptr;
    }

    void DeallocateTracked(void* ptr) {
        #ifdef ENABLE_TRACY_PROFILING
            PROFILE_FREE(ptr);
        #endif
        free(ptr);
    }
}
