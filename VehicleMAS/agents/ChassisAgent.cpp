#include "ChassisAgent.h"
#include <iostream>

namespace mas::agent
{
    ChassisAgent::ChassisAgent(std::shared_ptr<comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard): BaseAgent("ChassisAgent", broker, blackboard), m_model(model::ChassisConfig{}, false)
    {
    }

    void ChassisAgent::handleMessage(const comm::Message& msg)
    {
        if ((getState() == AgentState::Idle || getState() == AgentState::Done) && msg.type == comm::MessageType::Request)
        {
            std::cout << "[ChassisAgent] Получен запрос: " << msg.content << "\n";

            m_loadPercent = m_blackboard->read("load").value();
            int roadVal = static_cast<int>(m_blackboard->read("road").value());
            m_road = static_cast<RoadType>(roadVal);

            m_replyTo = msg.sender;
            setState(AgentState::Ready);
        }
    }

    void ChassisAgent::runIteration()
    {
        if (getState() == AgentState::Ready)
        {
            setState(AgentState::Working);

            m_lastPrediction = m_model.predict(m_state, m_loadPercent, m_road);

            std::cout << "[ChassisAgent] Прогноз: амортизаторы="
                << m_lastPrediction.shockerLife_hours << " ч\n";

            // Отправляем тип дороги EngineAgent
            std::string roadStr;
            switch (m_road)
            {
                case RoadType::Highway: 
                    roadStr = "smooth";  
                    break;
                case RoadType::City:    
                    roadStr = "normal";  
                    break;
                case RoadType::Gravel:  
                    roadStr = "rough";   
                    break;
                case RoadType::OffRoad: 
                    roadStr = "extreme"; 
                    break;
            }

            comm::Message toEngine;
            toEngine.type = comm::MessageType::Inform;
            toEngine.sender = m_id;
            toEngine.receiver = "EngineAgent";
            toEngine.content = "roadType=" + roadStr;
            sendMessage(toEngine);

            // Ответ координатору
            comm::Message reply;
            reply.type = comm::MessageType::Inform;
            reply.sender = m_id;
            reply.receiver = m_replyTo;
            reply.content = "Chassis done, shockerLife="
                + std::to_string(m_lastPrediction.shockerLife_hours)
                + ",bushingLife=" + std::to_string(m_lastPrediction.bushingLife_hours)
                + ",springLife=" + std::to_string(m_lastPrediction.springLife_hours);
            sendMessage(reply);

            setState(AgentState::Done);
        }
        else if (getState() == AgentState::Done)
        {
            setState(AgentState::Idle);
        }
    }
}