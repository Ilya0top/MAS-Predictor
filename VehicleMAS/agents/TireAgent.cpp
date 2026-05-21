#include "TireAgent.h"
#include <iostream>

namespace mas::agent
{
    TireAgent::TireAgent(std::shared_ptr<comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard): BaseAgent("TireAgent", broker, blackboard), m_model(model::TireConfig{}, false)
    {
    }

    void TireAgent::handleMessage(const comm::Message& msg)
    {
        if ((getState() == AgentState::Idle || getState() == AgentState::Done) && msg.type == comm::MessageType::Request)
        {
            std::cout << "[TireAgent] Получен запрос: " << msg.content << "\n";

            std::string content = msg.content;
            size_t posLoad = content.find("load=");
            size_t posRoad = content.find("road=");

            if (posLoad != std::string::npos)
            {
                size_t endLoad = content.find(',', posLoad);
                std::string loadStr = content.substr(posLoad + 5, endLoad - posLoad - 5);
                m_loadPercent = std::stod(loadStr);
            }
            if (posRoad != std::string::npos)
            {
                int roadVal = std::stoi(content.substr(posRoad + 5));
                m_road = static_cast<RoadType>(roadVal);
            }

            m_replyTo = msg.sender;
            setState(AgentState::Ready);
        }
    }

    void TireAgent::runIteration()
    {
        if (getState() == AgentState::Ready)
        {
            setState(AgentState::Working);

            m_lastPrediction = m_model.predict(m_state, m_loadPercent, m_road);

            std::cout << "[TireAgent] Прогноз: шины=" << m_lastPrediction.tireLife_hours << " ч\n";

            // Отправляем износ шин в BrakeAgent
            comm::Message toBrake;
            toBrake.type = comm::MessageType::Inform;
            toBrake.sender = m_id;
            toBrake.receiver = "BrakeAgent";
            toBrake.content = "tireWear=" + std::to_string(m_state.treadWear);
            sendMessage(toBrake);

            // Ответ координатору
            comm::Message reply;
            reply.type = comm::MessageType::Inform;
            reply.sender = m_id;
            reply.receiver = m_replyTo;
            reply.content = "Tire done, tireLife=" + std::to_string(m_lastPrediction.tireLife_hours);
            sendMessage(reply);

            setState(AgentState::Done);
        }
        else if (getState() == AgentState::Done)
        {
            setState(AgentState::Idle);
        }
    }
}