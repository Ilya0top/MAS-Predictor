#include "ChassisModel.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace mas::model
{
    ChassisModel::ChassisModel(const ChassisConfig& config, bool verbose) : m_config(config), m_verbose(verbose)
    {
    }

    void ChassisModel::log(const std::string& message) const
    {
        if (m_verbose)
            std::cout << "  [CHASSIS_VERBOSE] " << message << std::endl;
    }

    double ChassisModel::roadFactor(RoadType road) const
    {
        switch (road)
        {
        case RoadType::Highway:
            return 1.0;
        case RoadType::City:
            return 1.8;
        case RoadType::Gravel:
            return 3.5;
        case RoadType::OffRoad:
            return 7.0;
        default:
            return 1.0;
        }
    }

    double ChassisModel::calculateShockerWear(double loadPercent, RoadType road) const
    {
        double baseRate = m_config.maxShockerWear / m_config.maxShockerLife;
        double rFactor = roadFactor(road);
        double loadF = std::pow(loadPercent / 100.0, 1.2);

        double wear = baseRate * rFactor * loadF;

        log("ShockerWear: load=" + std::to_string(loadPercent) +
            " road=" + std::to_string(static_cast<int>(road)) +
            " baseRate=" + std::to_string(baseRate) +
            " roadFactor=" + std::to_string(rFactor) +
            " loadF=" + std::to_string(loadF) +
            " -> " + std::to_string(wear) + " %/ч");

        return wear;
    }

    double ChassisModel::calculateBushingWear(double loadPercent, RoadType road) const
    {
        double baseRate = m_config.maxBushingWear / m_config.maxBushingLife;
        double rFactor = roadFactor(road);
        double loadF = std::pow(loadPercent / 100.0, 1.0);
        double impactBonus = 1.0;

        if (road == RoadType::OffRoad)
            impactBonus = 1.5;
        else if (road == RoadType::Gravel)
            impactBonus = 1.2;

        double wear = baseRate * rFactor * loadF * impactBonus;

        log("BushingWear: load=" + std::to_string(loadPercent) +
            " road=" + std::to_string(static_cast<int>(road)) +
            " baseRate=" + std::to_string(baseRate) +
            " roadFactor=" + std::to_string(rFactor) +
            " loadF=" + std::to_string(loadF) +
            " impact=" + std::to_string(impactBonus) +
            " -> " + std::to_string(wear) + " мм/ч");

        return wear;
    }

    double ChassisModel::calculateSpringWear(double loadPercent, RoadType road) const
    {
        double baseRate = m_config.maxSpringWear / m_config.maxSpringLife;
        double rFactor = roadFactor(road);
        double loadF = std::pow(loadPercent / 100.0, 1.5);

        double wear = baseRate * rFactor * loadF;

        log("SpringWear: load=" + std::to_string(loadPercent) +
            " road=" + std::to_string(static_cast<int>(road)) +
            " baseRate=" + std::to_string(baseRate) +
            " roadFactor=" + std::to_string(rFactor) +
            " loadF=" + std::to_string(loadF) +
            " -> " + std::to_string(wear) + " %/ч");

        return wear;
    }

    ChassisPrediction ChassisModel::predict(const ChassisState& state, double loadPercent, RoadType road) const
    {
        ChassisPrediction result{};

        double shockerRate = calculateShockerWear(loadPercent, road);
        double bushingRate = calculateBushingWear(loadPercent, road);
        double springRate = calculateSpringWear(loadPercent, road);

        double shockerRemaining = m_config.maxShockerWear - state.shockerWear;
        double bushingRemaining = m_config.maxBushingWear - state.bushingWear;
        double springRemaining = m_config.maxSpringWear - state.springWear;

        result.shockerLife_hours = (shockerRate > 0.0) ? shockerRemaining / shockerRate : 1e9;
        result.bushingLife_hours = (bushingRate > 0.0) ? bushingRemaining / bushingRate : 1e9;
        result.springLife_hours = (springRate > 0.0) ? springRemaining / springRate : 1e9;

        result.shockerLife_hours = std::max(0.0, result.shockerLife_hours);
        result.bushingLife_hours = std::max(0.0, result.bushingLife_hours);
        result.springLife_hours = std::max(0.0, result.springLife_hours);

        double shockerRatio = state.shockerWear / m_config.maxShockerWear;
        double bushingRatio = state.bushingWear / m_config.maxBushingWear;
        double springRatio = state.springWear / m_config.maxSpringWear;
        result.isCritical = (shockerRatio >= 0.8) || (bushingRatio >= 0.8) || (springRatio >= 0.8);

        std::ostringstream rec;
        if (result.isCritical)
            rec << "КРИТИЧЕСКИЙ ИЗНОС! ";

        if (result.shockerLife_hours < 500.0)
            rec << "Замена амортизаторов через " << static_cast<int>(result.shockerLife_hours) << " ч. ";

        if (result.bushingLife_hours < 400.0)
            rec << "Замена сайлентблоков через " << static_cast<int>(result.bushingLife_hours) << " ч. ";

        if (result.springLife_hours < 1000.0)
            rec << "Замена пружин через " << static_cast<int>(result.springLife_hours) << " ч. ";

        if (rec.str().empty())
        {
            double minLife = std::min({ result.shockerLife_hours, result.bushingLife_hours, result.springLife_hours });
            rec << "Подвеска в норме. Ближайшее ТО через " << static_cast<int>(minLife) << " ч.";
        }
        result.recommendation = rec.str();

        log("PREDICT: shocker=" + std::to_string(result.shockerLife_hours) +
            " bushing=" + std::to_string(result.bushingLife_hours) +
            " spring=" + std::to_string(result.springLife_hours) +
            " critical=" + std::string(result.isCritical ? "YES" : "NO"));

        return result;
    }
}