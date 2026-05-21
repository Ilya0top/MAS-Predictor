#include "TireModel.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace mas::model
{
    TireModel::TireModel(const TireConfig& config, bool verbose): m_config(config), m_verbose(verbose)
    {
    }

    void TireModel::log(const std::string& message) const
    {
        if (m_verbose)
            std::cout << "  [TIRE_VERBOSE] " << message << std::endl;
    }

    double TireModel::calculateTreadWear(double loadPercent, RoadType road) const
    {
        double baseRate = m_config.maxTreadWear / m_config.maxTireLife;
        double loadF = std::pow(loadPercent / 100.0, 0.9);
        double roadF = 1.0;

        switch (road)
        {
            case RoadType::Highway: 
                roadF = 1.0;   
                break;
            case RoadType::City:    
                roadF = 1.8;   
                break;
            case RoadType::Gravel:  
                roadF = 4.0;  
                break;
            case RoadType::OffRoad: 
                roadF = 8.0;   
                break;
        }

        double wear = baseRate * loadF * roadF;

        log("TreadWear: load=" + std::to_string(loadPercent) +
            " road=" + std::to_string(static_cast<int>(road)) +
            " baseRate=" + std::to_string(baseRate) +
            " loadF=" + std::to_string(loadF) +
            " roadF=" + std::to_string(roadF) +
            " -> " + std::to_string(wear) + " ìì/÷");

        return wear;
    }

    TirePrediction TireModel::predict(const TireState& state, double loadPercent, RoadType road) const
    {
        TirePrediction result{};

        double wearRate = calculateTreadWear(loadPercent, road);
        double remaining = m_config.maxTreadWear - state.treadWear;

        result.tireLife_hours = (wearRate > 0.0) ? remaining / wearRate : 1e9;
        result.tireLife_hours = std::max(0.0, result.tireLife_hours);

        double wearRatio = state.treadWear / m_config.maxTreadWear;
        result.isCritical = wearRatio >= 0.85;

        std::ostringstream rec;
        if (result.isCritical)
            rec << "ÊÐÈÒÈ×ÅÑÊÈÉ ÈÇÍÎÑ ØÈÍ! ";

        if (result.tireLife_hours < 300.0)
            rec << "Çàìåíà øèí ÷åðåç " << static_cast<int>(result.tireLife_hours) << " ÷. ";

        if (rec.str().empty())
            rec << "Øèíû â íîðìå. Çàìåíà ÷åðåç " << static_cast<int>(result.tireLife_hours) << " ÷.";

        result.recommendation = rec.str();

        log("PREDICT: tireLife=" + std::to_string(result.tireLife_hours) + " critical=" + std::string(result.isCritical ? "YES" : "NO"));

        return result;
    }
}