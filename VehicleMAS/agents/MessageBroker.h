#pragma once
#include <string>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>
#include <memory>

namespace mas::comm
{
    enum class MessageType
    {
        Request,                    // запрос на расчёт
        Inform,                     // ответ с результатом
        Cfp,                        // Call for Proposal — запрос предложений
        Propose,                    // предложение
        Subscribe,                  // подписка на обновления
        Error                       // ошибка
    };

    struct Message
    {
        MessageType type;
        std::string sender;         // ID отправителя
        std::string receiver;       // ID получателя
        std::string content;        // данные
        int         requestId = 0;  // ID запроса для связи запрос-ответ
    };

    class MessageBroker
    {
    private:
        struct MessageQueue
        {
            std::queue<Message> messages;
            std::mutex mutex;
            std::condition_variable cv;
        };

        std::unordered_map<std::string, std::unique_ptr<MessageQueue>> m_queues;
        std::mutex m_mapMutex;

    public:
        MessageBroker() = default;

        // Отправить сообщение агенту
        void send(const Message& msg);

        // Получить сообщение для агента
        std::optional<Message> receive(const std::string& agentId, std::chrono::milliseconds timeout = std::chrono::milliseconds(100));

        // Зарегистрировать агента в брокере
        void registerAgent(const std::string& agentId);

        // Ждать и собрать N сообщений от разных отправителей
        std::vector<Message> receiveAll(const std::string& agentId, int expectedCount);
    };
}