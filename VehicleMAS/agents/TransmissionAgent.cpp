#include "TransmissionAgent.h"
#include <iostream>

namespace mas::agent
{
    TransmissionAgent::TransmissionAgent(std::shared_ptr<comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard): BaseAgent("TransmissionAgent", broker, blackboard), m_model(model::TransmissionConfig{}, false)
    {
    }

    void TransmissionAgent::handleMessage(const comm::Message& msg)
    {
        switch (getState())
        {
        case AgentState::Idle:
        case AgentState::Done:
            if (msg.type == comm::MessageType::Request)
            {
                std::cout << "[TransmissionAgent] Получен запрос: " << msg.content << "\n";

                m_loadPercent = m_blackboard->read("load").value();
                m_gearShifts = static_cast<int>(m_blackboard->read("shifts").value());

                m_replyTo = msg.sender;
                m_hasHeatFromEngine = false;
                m_hasCoolantTemp = false;
                setState(AgentState::Waiting);
            }
            break;

        case AgentState::Waiting:
            if (msg.sender == "EngineAgent" && msg.content.find("heatEmission=") != std::string::npos)
            {
                m_heatFromEngine = std::stod(msg.content.substr(msg.content.find("heatEmission=") + 13));
                m_hasHeatFromEngine = true;
                std::cout << "[TransmissionAgent] Получено тепло от Engine: "
                    << m_heatFromEngine << " кВт\n";
            }
            else if (msg.sender == "CoolingAgent" && msg.content.find("coolantTemp=") != std::string::npos)
            {
                m_coolantTemp = std::stod(msg.content.substr(msg.content.find("coolantTemp=") + 12));
                m_hasCoolantTemp = true;
                std::cout << "[TransmissionAgent] Получена tОЖ от Cooling: " << m_coolantTemp << "°C\n";
            }

            if (m_hasHeatFromEngine && m_hasCoolantTemp)
            {
                setState(AgentState::Ready);
            }
            break;

        default:
            break;
        }
    }

    void TransmissionAgent::runIteration()
    {
        if (getState() == AgentState::Ready)
        {
            setState(AgentState::Working);

            // Температура КПП = база + тепло от двигателя + влияние tОЖ
            m_state.oilTemperature = 80.0 + m_heatFromEngine * 0.1 + (m_coolantTemp - 80.0) * 0.4;

            m_lastPrediction = m_model.predict(m_state, m_loadPercent, m_gearShifts);

            std::cout << "[TransmissionAgent] Прогноз: фрикционы=" << m_lastPrediction.frictionLife_hours << " ч, tКПП=" << m_state.oilTemperature << "°C\n";

            // Отправляем температуру КПП в EngineAgent
            comm::Message toEngine;
            toEngine.type = comm::MessageType::Inform;
            toEngine.sender = m_id;
            toEngine.receiver = "EngineAgent";
            toEngine.content = "transOilTemp=" + std::to_string(m_state.oilTemperature);
            sendMessage(toEngine);

            // Ответ координатору
            comm::Message reply;
            reply.type = comm::MessageType::Inform;
            reply.sender = m_id;
            reply.receiver = m_replyTo;
            reply.content = "Transmission done, frictionLife="
                + std::to_string(m_lastPrediction.frictionLife_hours)
                + ",gearLife=" + std::to_string(m_lastPrediction.gearLife_hours)
                + ",oilLife=" + std::to_string(m_lastPrediction.oilLife_hours);
            sendMessage(reply);

            setState(AgentState::Done);
        }
        else if (getState() == AgentState::Done)
        {
            setState(AgentState::Idle);
        }
    }
}