#pragma once
#include "MessageBroker.h"
#include "Blackboard.h"
#include <string>
#include <memory>
#include <thread>
#include <atomic>

namespace mas::agent
{
    enum class AgentState
    {
        Idle,           // ничего не делает
        Waiting,        // ждёт данные от других агентов
        Ready,          // все данные получены, можно считать
        Working,        // выполняет расчёт
        Done            // расчёт завершён, результат отправлен
    };

    class BaseAgent
    {
    protected:
        std::string m_id;
        std::shared_ptr<comm::MessageBroker> m_broker;
        std::shared_ptr<comm::Blackboard> m_blackboard;
        std::jthread m_thread;
        std::atomic<bool> m_running{ false };
        std::atomic<AgentState> m_state{ AgentState::Idle };

        // Отправить сообщение
        void sendMessage(const mas::comm::Message& msg);

        // Обработать входящее сообщение
        virtual void handleMessage(const mas::comm::Message& msg) = 0;

        // Выполнить свою основную логику
        virtual void runIteration() = 0;

        // Главный цикл агента
        virtual void run(std::stop_token stoken);

        void setState(AgentState state);
        AgentState getState() const { return m_state; }

    public:
        BaseAgent(const std::string& id, std::shared_ptr<mas::comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard);
        virtual ~BaseAgent();

        // Запустить агента в отдельном потоке
        void start();

        // Остановить агента
        void stop();

        // ID агента
        const std::string& getId() const { return m_id; }
    };
}