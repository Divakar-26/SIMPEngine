#pragma once

#include <map>
#include <string>

namespace SIMPEngine
{
    class GameVariables
    {
    public:
        static double Get(const std::string &key, double fallback = 0.0);
        static void Set(const std::string &key, double value);
        static double Add(const std::string &key, double amount);
        static bool Has(const std::string &key);
        static void Remove(const std::string &key);
        static void Clear();

        static const std::map<std::string, double> &All();
        static void Replace(std::map<std::string, double> values);

        static std::string Format(const std::string &text);
    };

    class SaveData
    {
    public:
        static bool Save(const std::string &slot);
        static bool Load(const std::string &slot);
        static bool Exists(const std::string &slot);
        static bool Delete(const std::string &slot);
    };
}
