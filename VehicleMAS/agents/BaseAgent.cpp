#include "BaseAgent.h"
#include <iostream>

namespace mas::agent
{
    BaseAgent::BaseAgent(const std::string& id, std::shared_ptr<mas::comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard) : m_id(id), m_broker(broker), m_blackboard(blackboard)
    {
        m_broker->registerAgent(id);
    }

    BaseAgent::~BaseAgent()
    {
        stop();
    }

    void BaseAgent::start()
    {
        m_running = true;
        m_thread = std::jthread([this](std::stop_token stoken) {this->run(stoken); });
    }

    void BaseAgent::stop()
    {
        m_running = false;
        m_thread.request_stop();
        if (m_thread.joinable())
            m_thread.join();
    }

    void BaseAgent::sendMessage(const mas::comm::Message& msg)
    {
        m_broker->send(msg);
    }

    void BaseAgent::setState(AgentState state)
    {
        AgentState old = m_state.exchange(state);
    }

    void BaseAgent::run(std::stop_token stoken)
    {
        std::cout << "[Agent:" << m_id << "] Запущен\n";

        while (!stoken.stop_requested() && m_running)
        {
            auto msg = m_broker->receive(m_id, std::chrono::milliseconds(10));
            if (msg.has_value())
                handleMessage(msg.value());

            runIteration();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cout << "[Agent:" << m_id << "] Остановлен\n";
    }
}