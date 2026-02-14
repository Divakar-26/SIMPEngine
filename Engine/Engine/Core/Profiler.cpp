#include "Profiler.h"
#include <Core/Log.h>

ProfileTimer::ProfileTimer(const std::string & name) {
    m_Name = name;
    m_Start = std::chrono::high_resolution_clock::now();
}

ProfileTimer::~ProfileTimer(){
    auto end = std::chrono::high_resolution_clock::now();

    auto us = std::chrono::duration<double, std::micro>(end - m_Start).count();

    CORE_INFO("{} took {} ms", m_Name, us/1000.0);
}