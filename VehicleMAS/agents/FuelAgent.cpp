#include "FuelAgent.h"
#include <iostream>

namespace mas::agent
{
    FuelAgent::FuelAgent(std::shared_ptr<comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard): BaseAgent("FuelAgent", broker, blackboard), m_model(model::FuelSystemConfig{}, false)
    {
    }

    void FuelAgent::handleMessage(const comm::Message& msg)
    {
        if ((getState() == AgentState::Idle || getState() == AgentState::Done) && msg.type == comm::MessageType::Request)
        {
            std::cout << "[FuelAgent] Получен запрос: " << msg.content << "\n";

            m_loadPercent = m_blackboard->read("load").value();

            m_replyTo = msg.sender;
            setState(AgentState::Ready);
        }
    }

    void FuelAgent::runIteration()
    {
        if (getState() == AgentState::Ready)
        {
            setState(AgentState::Working);

            m_lastPrediction = m_model.predict(m_state, m_loadPercent);

            std::cout << "[FuelAgent] Прогноз: КПД=" << m_lastPrediction.efficiency << "%\n";

            // Отправляем КПД напрямую EngineAgent
            comm::Message toEngine;
            toEngine.type = comm::MessageType::Inform;
            toEngine.sender = m_id;
            toEngine.receiver = "EngineAgent";
            toEngine.content = "fuelEff=" + std::to_string(m_lastPrediction.efficiency);
            sendMessage(toEngine);

            // Ответ координатору
            comm::Message reply;
            reply.type = comm::MessageType::Inform;
            reply.sender = m_id;
            reply.receiver = m_replyTo;
            reply.content = "Fuel done, efficiency=" + std::to_string(m_lastPrediction.efficiency)
                + ",filterLife=" + std::to_string(m_lastPrediction.filterLife_hours)
                + ",injectorLife=" + std::to_string(m_lastPrediction.injectorLife_hours);
            sendMessage(reply);

            setState(AgentState::Done);
        }
        else if (getState() == AgentState::Done)
        {
            setState(AgentState::Idle);
        }
    }
}