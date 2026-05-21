#pragma once
#include <unordered_map>
#include <shared_mutex>
#include <condition_variable>
#include <optional>
#include <string>
#include <vector>

namespace mas::comm
{
    class Blackboard
    {
    private:
        std::unordered_map<std::string, double> m_data;
        mutable std::shared_mutex m_mutex;
        mutable std::condition_variable_any m_cv;

    public:
        void write(const std::string& key, double value)
        {
            std::unique_lock lock(m_mutex);
            m_data[key] = value;
            m_cv.notify_all();
        }

        std::optional<double> read(const std::string& key)
        {
            std::shared_lock lock(m_mutex);
            auto it = m_data.find(key);
            return (it != m_data.end()) ? std::optional{ it->second } : std::nullopt;
        }

        // Ждать, пока появятся все указанные ключи
        void waitFor(const std::vector<std::string>& keys)
        {
            std::shared_lock lock(m_mutex);
            m_cv.wait(lock, [&] {
                for (const auto& k : keys)
                    if (m_data.find(k) == m_data.end()) 
                        return false;

                return true;
            });
        }

        void clear()
        {
            std::unique_lock lock(m_mutex);
            m_data.clear();
        }
    };
}