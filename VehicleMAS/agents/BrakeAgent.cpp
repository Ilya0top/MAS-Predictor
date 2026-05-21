#include "BrakeAgent.h"
#include <iostream>

namespace mas::agent
{
    BrakeAgent::BrakeAgent(std::shared_ptr<comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard) : BaseAgent("BrakeAgent", broker, blackboard), m_model(model::BrakeConfig{}, false)
    {
    }

    void BrakeAgent::handleMessage(const comm::Message& msg)
    {
        switch (getState())
        {
        case AgentState::Idle:
        case AgentState::Done:
            if (msg.type == comm::MessageType::Request)
            {
                std::cout << "[BrakeAgent] Получен запрос: " << msg.content << "\n";

                std::string content = msg.content;
                size_t posLoad = content.find("load=");

                if (posLoad != std::string::npos)
                {
                    size_t endLoad = content.find(',', posLoad);
                    std::string loadStr = content.substr(posLoad + 5, endLoad - posLoad - 5);
                    m_loadPercent = std::stod(loadStr);
                }

                size_t posBrakes = content.find("brakes=");
                if (posBrakes != std::string::npos)
                    m_brakesPerHour = std::stoi(content.substr(posBrakes + 7));

                m_replyTo = msg.sender;
                m_hasTireWear = false;
                setState(AgentState::Waiting);
            }
            break;

        case AgentState::Waiting:
            if (msg.sender == "TireAgent" && msg.content.find("tireWear=") != std::string::npos)
            {
                m_tireWear = std::stod(msg.content.substr(msg.content.find("tireWear=") + 9));
                m_hasTireWear = true;
                std::cout << "[BrakeAgent] Износ шин: " << m_tireWear << " мм\n";
                setState(AgentState::Ready);
            }
            break;

        default:
            break;
        }
    }

    void BrakeAgent::runIteration()
    {
        if (getState() == AgentState::Ready)
        {
            setState(AgentState::Working);

            m_state.brakesPerHour = m_brakesPerHour;
            m_state.brakeTemp = 150.0 + m_tireWear * 15.0;

            m_lastPrediction = m_model.predict(m_state, m_loadPercent);

            std::cout << "[BrakeAgent] Прогноз: колодки=" << m_lastPrediction.padLife_hours
                << " ч, перегрев=" << (m_lastPrediction.isOverheat ? "ДА" : "НЕТ")
                << ", tТорм=" << m_state.brakeTemp << "°C\n";

            // Отправляем температуру тормозов в CoolingAgent
            comm::Message toCooling;
            toCooling.type = comm::MessageType::Inform;
            toCooling.sender = m_id;
            toCooling.receiver = "CoolingAgent";
            toCooling.content = "brakeTemp=" + std::to_string(m_state.brakeTemp);
            sendMessage(toCooling);

            // Ответ координатору
            comm::Message reply;
            reply.type = comm::MessageType::Inform;
            reply.sender = m_id;
            reply.receiver = m_replyTo;
            reply.content = "Brake done, padLife=" + std::to_string(m_lastPrediction.padLife_hours)
                + ",discLife=" + std::to_string(m_lastPrediction.discLife_hours);
            sendMessage(reply);

            setState(AgentState::Done);
        }
        else if (getState() == AgentState::Done)
        {
            setState(AgentState::Idle);
        }
    }
}