#include "MessageBroker.h"

namespace mas::comm
{
    void MessageBroker::registerAgent(const std::string& agentId)
    {
        std::lock_guard<std::mutex> lock(m_mapMutex);
        if (m_queues.find(agentId) == m_queues.end())
            m_queues[agentId] = std::make_unique<MessageQueue>();
    }

    void MessageBroker::send(const Message& msg)
    {
        std::lock_guard<std::mutex> mapLock(m_mapMutex);
        auto it = m_queues.find(msg.receiver);
        if (it == m_queues.end())
            return;

        auto& queue = it->second;
        {
            std::lock_guard<std::mutex> qLock(queue->mutex);
            queue->messages.push(msg);
        }
        queue->cv.notify_one();
    }

    std::optional<Message> MessageBroker::receive(const std::string& agentId, std::chrono::milliseconds timeout)
    {
        std::lock_guard<std::mutex> mapLock(m_mapMutex);
        auto it = m_queues.find(agentId);
        if (it == m_queues.end())
            return std::nullopt;

        auto& queue = it->second;
        std::unique_lock<std::mutex> qLock(queue->mutex);

        // сообщений нет
        if (!queue->cv.wait_for(qLock, timeout, [&queue] { return !queue->messages.empty(); }))
            return std::nullopt;


        Message msg = queue->messages.front();
        queue->messages.pop();
        return msg;
    }

    std::vector<Message> MessageBroker::receiveAll(const std::string& agentId, int expectedCount)
    {
        std::vector<Message> result;

        auto it = m_queues.find(agentId);
        if (it == m_queues.end())
            return result;

        auto& queue = it->second;

        while (result.size() < static_cast<size_t>(expectedCount))
        {
            {
                std::lock_guard<std::mutex> lock(queue->mutex);
                while (!queue->messages.empty() && result.size() < static_cast<size_t>(expectedCount))
                {
                    result.push_back(queue->messages.front());
                    queue->messages.pop();
                }
            }

            if (result.size() >= static_cast<size_t>(expectedCount))
                break;

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        return result;
    }
}