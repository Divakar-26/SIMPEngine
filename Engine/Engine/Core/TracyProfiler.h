#pragma once

/*
    Tracy Profiler Integration for SIMPEngine
    
    Features:
    - Zero-overhead when disabled (compile-time)
    - Real-time frame profiling
    - Memory tracking
    - Lock contention analysis
    - Works on Linux, Windows, macOS
    
    Usage:
        ZoneScopedN("FunctionName");  // Zone profiling
        FrameMark;                    // Mark frame boundaries
        TracyAlloc(ptr, size);        // Track allocations
        TracyFree(ptr);               // Track deallocations
*/

#ifdef ENABLE_TRACY_PROFILING
    #include <tracy/Tracy.hpp>
    #define PROFILE_FRAME() FrameMark
    #define PROFILE_SCOPE(name) ZoneScoped; ZoneName(name, strlen(name))
    #define PROFILE_SCOPE_N(name) ZoneScopedN(name)
    #define PROFILE_FUNCTION() ZoneScoped
    #define PROFILE_ALLOC(ptr, size) TracyAlloc(ptr, size)
    #define PROFILE_FREE(ptr) TracyFree(ptr)
    #define PROFILE_MESSAGE(txt, size) TracyMessage(txt, size)
    #define PROFILE_PLOT(name, val) TracyPlot(name, val)
#else
    #define PROFILE_FRAME()
    #define PROFILE_SCOPE(name)
    #define PROFILE_SCOPE_N(name)
    #define PROFILE_FUNCTION()
    #define PROFILE_ALLOC(ptr, size)
    #define PROFILE_FREE(ptr)
    #define PROFILE_MESSAGE(txt, size)
    #define PROFILE_PLOT(name, val)
#endif

namespace SIMPEngine::Profiling {
    // Lightweight profiler statistics
    struct FrameStats {
        float frameTime = 0.0f;      // ms
        float fps = 0.0f;
        int drawCalls = 0;
        int entityCount = 0;
        size_t memoryUsage = 0;      // bytes
    };

    extern FrameStats g_FrameStats;

    // Initialize Tracy if enabled
    void InitProfiler();

    // Get frame statistics
    FrameStats GetFrameStats();

    // Update frame statistics
    void UpdateFrameStats(float dt, int drawCalls, int entityCount);

    // Memory tracking
    void* AllocateTracked(size_t size, const char* label = nullptr);
    void DeallocateTracked(void* ptr);
}
