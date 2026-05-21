#include "TransmissionModel.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace mas::model
{
    TransmissionModel::TransmissionModel(const TransmissionConfig& config, bool verbose) : m_config(config), m_verbose(verbose)
    {
    }

    void TransmissionModel::log(const std::string& message) const
    {
        if (m_verbose)
            std::cout << "  [TRANS_VERBOSE] " << message << std::endl;
    }

    double TransmissionModel::calculateFrictionWear(double loadPercent, double oilTemp, double oilQuality, int gearShiftsPerHour) const
    {
        double baseRate = m_config.maxFrictionWear / m_config.maxLifeHours;
        double loadF = loadPercent / 100.0;
        double tempFactor = 1.0;

        if (oilTemp > 80.0)
        {
            double overheat = (oilTemp - 80.0) / (m_config.maxOilTemp - 80.0);
            overheat = std::clamp(overheat, 0.0, 1.0);
            tempFactor = 1.0 + 3.0 * std::pow(overheat, 1.5);
        }

        double quality = std::clamp(oilQuality, 5.0, 100.0);
        double oilFactor = 1.0 + 2.0 * (1.0 - quality / 100.0);
        double shiftFactor = 1.0 + gearShiftsPerHour * 0.05;

        double wear = baseRate * loadF * tempFactor * oilFactor * shiftFactor;

        log("FrictionWear: load=" + std::to_string(loadPercent) +
            " oilTemp=" + std::to_string(oilTemp) +
            " oilQ=" + std::to_string(oilQuality) +
            " shifts=" + std::to_string(gearShiftsPerHour) +
            " baseRate=" + std::to_string(baseRate) +
            " loadF=" + std::to_string(loadF) +
            " tempFactor=" + std::to_string(tempFactor) +
            " oilFactor=" + std::to_string(oilFactor) +
            " shiftFactor=" + std::to_string(shiftFactor) +
            " -> " + std::to_string(wear) + " мм/ч");

        return wear;
    }

    double TransmissionModel::calculateGearWear(double loadPercent, double oilTemp, double oilQuality) const
    {
        double baseRate = m_config.maxGearWear / m_config.maxLifeHours;
        double loadF = std::pow(loadPercent / 100.0, 0.9);
        double tempFactor = 1.0;

        if (oilTemp > 80.0)
        {
            double overheat = (oilTemp - 80.0) / (m_config.maxOilTemp - 80.0);
            overheat = std::clamp(overheat, 0.0, 1.0);
            tempFactor = 1.0 + 2.5 * std::pow(overheat, 1.3);
        }

        double quality = std::clamp(oilQuality, 5.0, 100.0);
        double oilFactor = 1.0 + 1.2 * (1.0 - quality / 100.0);

        double wear = baseRate * loadF * tempFactor * oilFactor;

        log("GearWear: load=" + std::to_string(loadPercent) +
            " oilTemp=" + std::to_string(oilTemp) +
            " oilQ=" + std::to_string(oilQuality) +
            " baseRate=" + std::to_string(baseRate) +
            " loadF=" + std::to_string(loadF) +
            " tempFactor=" + std::to_string(tempFactor) +
            " oilFactor=" + std::to_string(oilFactor) +
            " -> " + std::to_string(wear) + " мм/ч");

        return wear;
    }

    double TransmissionModel::calculateOilDegradation(double oilTemp, double loadPercent) const
    {
        constexpr double BASE_DEGRADATION = 100.0 / 500.0;
        double tempFactor = 1.0;

        if (oilTemp > 80.0)
        {
            double overheat = (oilTemp - 80.0) / (m_config.maxOilTemp - 80.0);
            overheat = std::clamp(overheat, 0.0, 1.0);
            tempFactor = 1.0 + 3.5 * std::pow(overheat, 1.6);
        }

        double loadF = std::pow(loadPercent / 100.0, 0.5);

        double degradation = BASE_DEGRADATION * tempFactor * loadF;

        log("OilDegradation: t=" + std::to_string(oilTemp) +
            " load=" + std::to_string(loadPercent) +
            " tempFactor=" + std::to_string(tempFactor) +
            " loadF=" + std::to_string(loadF) +
            " -> " + std::to_string(degradation) + " %/ч");

        return degradation;
    }

    TransmissionPrediction TransmissionModel::predict(const TransmissionState& state, double loadPercent, int gearShiftsPerHour) const
    {
        TransmissionPrediction result{};

        double frictionRate = calculateFrictionWear(loadPercent, state.oilTemperature, state.oilQuality, gearShiftsPerHour);
        double gearRate = calculateGearWear(loadPercent, state.oilTemperature, state.oilQuality);
        double oilDegRate = calculateOilDegradation(state.oilTemperature, loadPercent);

        double frictionRemaining = m_config.maxFrictionWear - state.frictionWear;
        double gearRemaining = m_config.maxGearWear - state.gearWear;
        double oilRemaining = state.oilQuality - 20.0;

        result.frictionLife_hours = (frictionRate > 0.0) ? frictionRemaining / frictionRate : 1e9;
        result.gearLife_hours = (gearRate > 0.0) ? gearRemaining / gearRate : 1e9;
        result.oilLife_hours = (oilDegRate > 0.0) ? oilRemaining / oilDegRate : 1e9;

        result.frictionLife_hours = std::max(0.0, result.frictionLife_hours);
        result.gearLife_hours = std::max(0.0, result.gearLife_hours);
        result.oilLife_hours = std::max(0.0, result.oilLife_hours);

        double frictionRatio = state.frictionWear / m_config.maxFrictionWear;
        double gearRatio = state.gearWear / m_config.maxGearWear;
        result.isCritical = (frictionRatio >= 0.8) || (gearRatio >= 0.8) || (state.oilQuality <= 20.0);


        double minLife = std::min({ result.frictionLife_hours, result.gearLife_hours, result.oilLife_hours });

        std::ostringstream rec;
        if (result.isCritical)
            rec << "КРИТИЧЕСКОЕ СОСТОЯНИЕ! ";

        if (result.oilLife_hours < 100.0)
            rec << "Замена масла КПП через " << static_cast<int>(result.oilLife_hours) << " ч. ";

        if (result.frictionLife_hours < 500.0)
            rec << "Замена фрикционов через " << static_cast<int>(result.frictionLife_hours) << " ч. ";

        if (result.gearLife_hours < 800.0)
            rec << "Ремонт шестерён через " << static_cast<int>(result.gearLife_hours) << " ч. ";

        if (rec.str().empty())
            rec << "Трансмиссия в норме. Замена масла через " << static_cast<int>(minLife) << " ч.";

        result.recommendation = rec.str();

        log("PREDICT: frictionLife=" + std::to_string(result.frictionLife_hours) +
            " gearLife=" + std::to_string(result.gearLife_hours) +
            " oilLife=" + std::to_string(result.oilLife_hours) +
            " critical=" + std::string(result.isCritical ? "YES" : "NO"));

        return result;
    }
}