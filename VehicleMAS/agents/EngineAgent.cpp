#include "EngineAgent.h"
#include <iostream>
#include <sstream>

namespace mas::agent
{
    EngineAgent::EngineAgent(std::shared_ptr<mas::comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard): BaseAgent("EngineAgent", broker, blackboard), m_model(mas::model::EngineConfig{}, false)
    {
    }

    void EngineAgent::handleMessage(const comm::Message& msg)
    {
        switch (getState())
        {
        case AgentState::Idle:
            if (msg.type == comm::MessageType::Request)
            {
                std::cout << "[EngineAgent] Получен запрос: " << msg.content << "\n";
                m_loadPercent = std::stod(msg.content.substr(msg.content.find("load=") + 5));
                m_replyTo = msg.sender;
                m_transOilTemp = 80.0;
                m_hasFuelEff = false;
                m_hasCoolant = false;
                m_hasVibration = false;
                setState(AgentState::Waiting);
            }
            break;

        case AgentState::Done:
            if (msg.sender == "TransmissionAgent" && msg.content.find("transOilTemp=") != std::string::npos)
            {
                m_transOilTemp = std::stod(msg.content.substr(msg.content.find("transOilTemp=") + 13));
                std::cout << "[EngineAgent] Получена tКПП от Transmission: " << m_transOilTemp << "°C\n";
            }
            else if (msg.type == comm::MessageType::Request)
            {
                std::cout << "[EngineAgent] Получен запрос: " << msg.content << "\n";
                m_loadPercent = std::stod(msg.content.substr(msg.content.find("load=") + 5));
                m_replyTo = msg.sender;
                m_transOilTemp = 80.0;
                m_hasFuelEff = false;
                m_hasCoolant = false;
                m_hasVibration = false;
                setState(AgentState::Waiting);
            }
            break;

        case AgentState::Waiting:
            if (msg.sender == "FuelAgent" && msg.content.find("fuelEff=") != std::string::npos)
            {
                m_fuelEfficiency = std::stod(msg.content.substr(msg.content.find("fuelEff=") + 8));
                m_hasFuelEff = true;
                std::cout << "[EngineAgent] Получен КПД от FuelAgent: " << m_fuelEfficiency << "%\n";
            }
            else if (msg.sender == "CoolingAgent" && msg.content.find("coolantTemp=") != std::string::npos)
            {
                m_coolantTemp = std::stod(msg.content.substr(msg.content.find("coolantTemp=") + 12));
                m_hasCoolant = true;
                std::cout << "[EngineAgent] Получена tОЖ от CoolingAgent: " << m_coolantTemp << "°C\n";
            }
            else if (msg.sender == "ChassisAgent" && msg.content.find("roadType=") != std::string::npos)
            {
                std::string road = msg.content.substr(msg.content.find("roadType=") + 9);
                if (road == "smooth")       m_vibrationFactor = 1.0;
                else if (road == "normal")  m_vibrationFactor = 1.1;
                else if (road == "rough")   m_vibrationFactor = 1.3;
                else if (road == "extreme") m_vibrationFactor = 1.6;
                m_hasVibration = true;
                std::cout << "[EngineAgent] Вибрация от дороги: " << road << " (x" << m_vibrationFactor << ")\n";
            }

            if (m_hasFuelEff && m_hasCoolant && m_hasVibration)
            {
                setState(AgentState::Ready);
            }
            break;

        default:
            break;
        }
    }

    void EngineAgent::runIteration()
    {
        switch (getState())
        {
        case AgentState::Ready:
        {
            setState(AgentState::Working);

            // Используем tОЖ от CoolingAgent как температуру масла
            m_state.oilTemperature = m_coolantTemp * m_vibrationFactor + (m_transOilTemp - 80.0) * 0.3;

            m_lastPrediction = m_model.predict(m_state, m_loadPercent, m_fuelEfficiency);

            std::cout << "[EngineAgent] Прогноз: мощность="
                << m_lastPrediction.currentPower_kW << " кВт, "
                << "ресурс колец=" << m_lastPrediction.pistonLife_hours << " ч\n";

            m_blackboard->write("enginePistonLife", m_lastPrediction.pistonLife_hours);
            m_blackboard->write("enginePower", m_lastPrediction.currentPower_kW);
            m_blackboard->write("engineFuelConsumption", m_lastPrediction.fuelConsumption);
            m_blackboard->write("engineHeatEmission", m_lastPrediction.heatEmission_kW);

            // Отправляем тепловыделение в TransmissionAgent
            comm::Message toTrans;
            toTrans.type = comm::MessageType::Inform;
            toTrans.sender = m_id;
            toTrans.receiver = "TransmissionAgent";
            toTrans.content = "heatEmission=" + std::to_string(m_lastPrediction.heatEmission_kW);
            sendMessage(toTrans);

            // Отвечаем координатору
            comm::Message reply;
            reply.type = comm::MessageType::Inform;
            reply.sender = m_id;
            reply.receiver = m_replyTo;
            reply.content = "Engine done, pistonLife="
                + std::to_string(m_lastPrediction.pistonLife_hours)
                + ",cylinderLife=" + std::to_string(m_lastPrediction.cylinderLife_hours)
                + ",oilLife=" + std::to_string(m_lastPrediction.oilLife_hours)
                + ",power=" + std::to_string(m_lastPrediction.currentPower_kW)
                + ",fuelConsumption=" + std::to_string(m_lastPrediction.fuelConsumption)
                + ",heatEmission=" + std::to_string(m_lastPrediction.heatEmission_kW);
            sendMessage(reply);

            setState(AgentState::Done);
            break;
        }
        case AgentState::Done:
            setState(AgentState::Idle);
            break;
        default:
            break;
        }
    }
}