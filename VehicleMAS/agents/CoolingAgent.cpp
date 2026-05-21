#include "CoolingAgent.h"
#include <iostream>

namespace mas::agent
{
    CoolingAgent::CoolingAgent(std::shared_ptr<comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard): BaseAgent("CoolingAgent", broker, blackboard), m_model(model::CoolingConfig{}, false)
    {
    }

    void CoolingAgent::handleMessage(const comm::Message& msg)
    {
        switch (getState())
        {
        case AgentState::Idle:
        case AgentState::Done:
            if (msg.type == comm::MessageType::Request)
            {
                std::cout << "[CoolingAgent] Получен запрос: " << msg.content << "\n";

                std::string content = msg.content;
                size_t posLoad = content.find("load=");
                size_t posTemp = content.find("extTemp=");

                if (posLoad != std::string::npos)
                {
                    size_t endLoad = content.find(',', posLoad);
                    std::string loadStr = content.substr(posLoad + 5, endLoad - posLoad - 5);
                    m_loadPercent = std::stod(loadStr);
                }
                if (posTemp != std::string::npos)
                    m_externalTemp = std::stod(content.substr(posTemp + 8));

                m_replyTo = msg.sender;
                m_hasBrakeHeat = false;
                setState(AgentState::Waiting);
            }
            break;

        case AgentState::Waiting:
            if (msg.sender == "BrakeAgent" && msg.content.find("brakeTemp=") != std::string::npos)
            {
                m_brakeHeat = std::stod(msg.content.substr(msg.content.find("brakeTemp=") + 10));
                m_hasBrakeHeat = true;
                std::cout << "[CoolingAgent] Нагрев от тормозов: " << m_brakeHeat << "°C\n";
                setState(AgentState::Ready);
            }
            break;

        default:
            break;
        }
    }

    void CoolingAgent::runIteration()
    {
        if (getState() == AgentState::Ready)
        {
            setState(AgentState::Working);

            m_lastPrediction = m_model.predict(m_state, m_loadPercent, m_externalTemp);
            m_lastPrediction.predictedCoolantTemp += m_brakeHeat * 0.03;

            std::cout << "[CoolingAgent] Прогноз: tОЖ="
                << m_lastPrediction.predictedCoolantTemp << "°C\n";

            comm::Message toEngine;
            toEngine.type = comm::MessageType::Inform;
            toEngine.sender = m_id;
            toEngine.receiver = "EngineAgent";
            toEngine.content = "coolantTemp=" + std::to_string(m_lastPrediction.predictedCoolantTemp);
            sendMessage(toEngine);

            comm::Message toTrans;
            toTrans.type = comm::MessageType::Inform;
            toTrans.sender = m_id;
            toTrans.receiver = "TransmissionAgent";
            toTrans.content = "coolantTemp=" + std::to_string(m_lastPrediction.predictedCoolantTemp);
            sendMessage(toTrans);

            comm::Message reply;
            reply.type = comm::MessageType::Inform;
            reply.sender = m_id;
            reply.receiver = m_replyTo;
            reply.content = "Cooling done, coolantTemp="
                + std::to_string(m_lastPrediction.predictedCoolantTemp)
                + ",coolantLife=" + std::to_string(m_lastPrediction.coolantLife_hours)
                + ",pumpLife=" + std::to_string(m_lastPrediction.pumpLife_hours);
            sendMessage(reply);

            setState(AgentState::Done);
        }
        else if (getState() == AgentState::Done)
        {
            setState(AgentState::Idle);
        }
    }
}