#include "CoordinatorAgent.h"
#include <iostream>
#include <algorithm>
#include <iomanip>

namespace mas::agent
{
    CoordinatorAgent::CoordinatorAgent(std::shared_ptr<comm::MessageBroker> broker, std::shared_ptr<comm::Blackboard> blackboard): BaseAgent("CoordinatorAgent", broker, blackboard)
    {
    }

    void CoordinatorAgent::handleMessage(const comm::Message& msg)
    {
        if (msg.type == comm::MessageType::Inform && m_collecting)
            parseResponse(msg.sender, msg.content);
    }

    void CoordinatorAgent::runIteration()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void CoordinatorAgent::run(std::stop_token stoken)
    {
        std::cout << "[Agent:" << m_id << "] Запущен\n";

        while (!stoken.stop_requested() && m_running)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::cout << "[Agent:" << m_id << "] Остановлен\n";
    }

    CombinedPrediction CoordinatorAgent::runPrediction(const Scenario& scenario)
    {
        m_blackboard->clear();
        m_collecting = true;
        m_combined = CombinedPrediction{};

        std::cout << "\n[Coordinator] Запуск прогноза (Actor Model)\n";

        m_blackboard->write("load", scenario.loadPercent);
        m_blackboard->write("shifts", scenario.gearShiftsPerHour);
        m_blackboard->write("brakes", scenario.brakesPerHour);
        m_blackboard->write("road", static_cast<int>(scenario.road));
        m_blackboard->write("extTemp", scenario.externalTemp);

        comm::Message req;
        req.type = comm::MessageType::Request;
        req.sender = m_id;
        req.content = "check";

        req.receiver = "EngineAgent";       sendMessage(req);
        req.receiver = "TransmissionAgent"; sendMessage(req);
        req.receiver = "BrakeAgent";        sendMessage(req);
        req.receiver = "ChassisAgent";      sendMessage(req);
        req.receiver = "CoolingAgent";      sendMessage(req);
        req.receiver = "FuelAgent";         sendMessage(req);
        req.receiver = "TireAgent";         sendMessage(req);

        auto replies = m_broker->receiveAll(m_id, 7);
        for (const auto& r : replies)
            parseResponse(r.sender, r.content);

        m_collecting = false;
        return buildSummary();
    }

    void CoordinatorAgent::parseResponse(const std::string& sender, const std::string& content)
    {
        m_responsesReceived++;

        if (sender == "EngineAgent")
        {
            size_t pos1 = content.find("pistonLife=");
            size_t pos2 = content.find("power=");
            size_t pos3 = content.find("fuelConsumption=");
            size_t pos4 = content.find("heatEmission=");
            size_t posCyl = content.find("cylinderLife=");
            size_t posOil = content.find("oilLife=");

            if (pos1 != std::string::npos)
            {
                size_t end1 = content.find(',', pos1);
                std::string val = (end1 != std::string::npos)
                    ? content.substr(pos1 + 11, end1 - pos1 - 11)
                    : content.substr(pos1 + 11);
                m_combined.engine.pistonLife_hours = std::stod(val);
            }
            if (pos2 != std::string::npos)
            {
                size_t end2 = content.find(',', pos2);
                std::string val = (end2 != std::string::npos)
                    ? content.substr(pos2 + 6, end2 - pos2 - 6)
                    : content.substr(pos2 + 6);
                m_combined.engine.currentPower_kW = std::stod(val);
            }
            if (pos3 != std::string::npos)
            {
                size_t end3 = content.find(',', pos3);
                std::string val = (end3 != std::string::npos)
                    ? content.substr(pos3 + 16, end3 - pos3 - 16)
                    : content.substr(pos3 + 16);
                m_combined.engine.fuelConsumption = std::stod(val);
            }
            if (pos4 != std::string::npos)
            {
                std::string val = content.substr(pos4 + 13);
                m_combined.engine.heatEmission_kW = std::stod(val);
            }
            if (posCyl != std::string::npos)
            {
                size_t endCyl = content.find(',', posCyl);
                std::string val = (endCyl != std::string::npos)
                    ? content.substr(posCyl + 13, endCyl - posCyl - 13)
                    : content.substr(posCyl + 13);
                m_combined.engine.cylinderLife_hours = std::stod(val);
            }
            if (posOil != std::string::npos)
            {
                size_t endOil = content.find(',', posOil);
                std::string val = (endOil != std::string::npos)
                    ? content.substr(posOil + 8, endOil - posOil - 8)
                    : content.substr(posOil + 8);
                m_combined.engine.oilLife_hours = std::stod(val);
            }
        }
        else if (sender == "TransmissionAgent")
        {
            size_t pos1 = content.find("frictionLife=");
            size_t pos2 = content.find("oilLife=");
            size_t pos3 = content.find("gearLife=");
            if (pos1 != std::string::npos)
                m_combined.transmission.frictionLife_hours = std::stod(content.substr(pos1 + 13));
            if (pos2 != std::string::npos)
                m_combined.transmission.oilLife_hours = std::stod(content.substr(pos2 + 8));
            if (pos3 != std::string::npos)
                m_combined.transmission.gearLife_hours = std::stod(content.substr(pos3 + 9));
        }
        else if (sender == "BrakeAgent")
        {
            size_t pos1 = content.find("padLife=");
            size_t pos2 = content.find("discLife=");
            if (pos1 != std::string::npos)
                m_combined.brakes.padLife_hours = std::stod(content.substr(pos1 + 8));
            if (pos2 != std::string::npos)
                m_combined.brakes.discLife_hours = std::stod(content.substr(pos2 + 9));
        }
        else if (sender == "ChassisAgent")
        {
            size_t pos1 = content.find("shockerLife=");
            size_t pos2 = content.find("bushingLife=");
            size_t pos3 = content.find("springLife=");
            if (pos1 != std::string::npos)
                m_combined.chassis.shockerLife_hours = std::stod(content.substr(pos1 + 12));
            if (pos2 != std::string::npos)
                m_combined.chassis.bushingLife_hours = std::stod(content.substr(pos2 + 12));
            if (pos3 != std::string::npos)
                m_combined.chassis.springLife_hours = std::stod(content.substr(pos3 + 11));
        }
        else if (sender == "CoolingAgent")
        {
            size_t pos1 = content.find("coolantTemp=");
            size_t pos2 = content.find("coolantLife=");
            size_t pos3 = content.find("pumpLife=");
            if (pos1 != std::string::npos)
                m_combined.cooling.predictedCoolantTemp = std::stod(content.substr(pos1 + 12));
            if (pos2 != std::string::npos)
                m_combined.cooling.coolantLife_hours = std::stod(content.substr(pos2 + 12));
            if (pos3 != std::string::npos)
                m_combined.cooling.pumpLife_hours = std::stod(content.substr(pos3 + 9));
        }
        else if (sender == "FuelAgent")
        {
            size_t pos1 = content.find("efficiency=");
            size_t pos2 = content.find("filterLife=");
            size_t pos3 = content.find("injectorLife=");
            if (pos1 != std::string::npos)
                m_combined.fuel.efficiency = std::stod(content.substr(pos1 + 11));
            if (pos2 != std::string::npos)
                m_combined.fuel.filterLife_hours = std::stod(content.substr(pos2 + 11));
            if (pos3 != std::string::npos)
                m_combined.fuel.injectorLife_hours = std::stod(content.substr(pos3 + 13));
        }
        else if (sender == "TireAgent")
        {
            size_t pos = content.find("tireLife=");
            if (pos != std::string::npos)
                m_combined.tires.tireLife_hours = std::stod(content.substr(pos + 9));
        }
    }

    CombinedPrediction CoordinatorAgent::buildSummary()
    {
        std::vector<double> lives = {
            m_combined.engine.pistonLife_hours,
            m_combined.transmission.frictionLife_hours,
            m_combined.brakes.padLife_hours,
            m_combined.chassis.shockerLife_hours,
            m_combined.cooling.coolantLife_hours,
            m_combined.fuel.filterLife_hours,
            m_combined.tires.tireLife_hours
        };

        m_combined.minLifeHours = *std::min_element(lives.begin(), lives.end());

        std::ostringstream sum;
        sum << "Сводный прогноз:\n";
        sum << "  Двигатель: кольца=" << static_cast<int>(m_combined.engine.pistonLife_hours) << " ч"
            << ", мощность=" << static_cast<int>(m_combined.engine.currentPower_kW) << " кВт"
            << ", расход=" << std::fixed << std::setprecision(1)
            << m_combined.engine.fuelConsumption << std::setprecision(0) << " л/ч\n";
        sum << "  Трансмиссия: фрикционы=" << static_cast<int>(m_combined.transmission.frictionLife_hours) << " ч\n";
        sum << "  Тормоза: колодки=" << static_cast<int>(m_combined.brakes.padLife_hours) << " ч\n";
        sum << "  Подвеска: амортизаторы=" << static_cast<int>(m_combined.chassis.shockerLife_hours) << " ч\n";
        sum << "  Охлаждение: tОЖ=" << static_cast<int>(m_combined.cooling.predictedCoolantTemp) << "°C\n";
        sum << "  Топливная: КПД=" << static_cast<int>(m_combined.fuel.efficiency) << "%\n";
        sum << "  Шины: " << static_cast<int>(m_combined.tires.tireLife_hours) << " ч\n";
        sum << "\n  >>> Ближайшее ТО через " << static_cast<int>(m_combined.minLifeHours) << " ч";

        m_combined.summary = sum.str();
        return m_combined;
    }
}