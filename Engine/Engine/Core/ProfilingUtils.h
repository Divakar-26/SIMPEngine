#pragma once

#include <chrono>
#include <string>

/*
    Convenient profiling utilities for SIMPEngine (Tracy profiler disabled)
    
    Usage:
        {
            PROFILE_TIMER("MyExpensiveOperation");
            // ... code to profile
        }  // Results printed on scope exit
*/

namespace SIMPEngine::Profiling {

    class ScopeTimer {
    public:
        explicit ScopeTimer(const std::string& name) 
            : m_Name(name) {
            m_Start = std::chrono::high_resolution_clock::now();
        }

        ~ScopeTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - m_Start).count();
            CORE_TRACE("{}: {:.3f} ms", m_Name, duration);
        }

    private:
        std::string m_Name;
        std::chrono::high_resolution_clock::time_point m_Start;
    };

}

// Convenience macro for RAII-based profiling
#define PROFILE_TIMER(name) \
    ::SIMPEngine::Profiling::ScopeTimer _profile_timer(name)

// Disabled profiling macros (Tracy removed)
#define PROFILE_FUNCTION()
#define PROFILE_FUNCTION_N(name)
#define PROFILE_SCOPE_N(name)
#define PROFILE_FRAME()
#define PROFILE_RENDER_SYSTEM()
#define PROFILE_PHYSICS_UPDATE()
#define PROFILE_COLLISION_DETECTION()
#define PROFILE_ANIMATION_UPDATE()
#define PROFILE_CAMERA_UPDATE()
#define PROFILE_HIERARCHY_UPDATE()
