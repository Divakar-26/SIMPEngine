#pragma once
#include <string>   
#include <chrono>

class ProfileTimer{
    public:
        ProfileTimer(const std::string & name);
        ~ProfileTimer();

    private:    

        std::string m_Name;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_Start; 
};