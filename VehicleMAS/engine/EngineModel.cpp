#include "EngineModel.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace mas::model
{
    EngineModel::EngineModel(const EngineConfig& config, bool verbose) : m_config(config), m_verbose(verbose)
    {
    }

    void EngineModel::log(const std::string& message) const
    {
        if (m_verbose)
            std::cout << "  [VERBOSE] " << message << std::endl;
    }

    double EngineModel::temperatureFactor(double oilTemp) const
    {
        constexpr double nominalTemp = 90.0;

        if (oilTemp <= nominalTemp)
            return 1.0;

        double overheat = (oilTemp - nominalTemp) / (m_config.maxOilTemp - nominalTemp);

        if (overheat > 1.0)
            overheat = 1.0 + (overheat - 1.0) * 0.3;

        double factor = 1.0 + 4.0 * std::pow(overheat, 1.8);

        log("temperatureFactor: t=" + std::to_string(oilTemp) +
            " overheat=" + std::to_string(overheat) +
            " factor=" + std::to_string(factor));

        return factor;
    }

    double EngineModel::oilQualityFactor(double oilQuality) const
    {
        double quality = std::clamp(oilQuality, 5.0, 100.0);
        double factor = 1.0 + 1.5 * (1.0 - quality / 100.0);

        log("oilQualityFactor: q=" + std::to_string(oilQuality) +
            " factor=" + std::to_string(factor));

        return factor;
    }

    double EngineModel::loadFactor(double loadPercent) const
    {
        return std::clamp(loadPercent / 100.0, 0.0, 1.0);
    }

    double EngineModel::calculatePistonWear(double loadPercent, double oilTemp, double oilQuality) const
    {
        double baseRate = m_config.maxWear / m_config.maxLifeHours;
        double tempFactor = temperatureFactor(oilTemp);
        double oilFactor = oilQualityFactor(oilQuality);
        double loadF = loadFactor(loadPercent);

        double wear = baseRate * loadF * tempFactor * oilFactor;

        log("PistonWear: load=" + std::to_string(loadPercent) +
            " t=" + std::to_string(oilTemp) +
            " oilQ=" + std::to_string(oilQuality) +
            " -> " + std::to_string(wear) + " мм/ч");

        return wear;
    }

    double EngineModel::calculateCylinderWear(double loadPercent, double oilTemp, double oilQuality) const
    {
        double baseRate = m_config.maxWear / m_config.maxLifeHours;
        double tempFactor = temperatureFactor(oilTemp);
        double oilFactor = oilQualityFactor(oilQuality);


        if (oilTemp > 90.0)
        {
            double overheat = (oilTemp - 90.0) / (m_config.maxOilTemp - 90.0);
            overheat = std::clamp(overheat, 0.0, 1.0);
            tempFactor *= (1.0 + std::pow(overheat, 1.2));
        }

        double loadF = std::pow(loadFactor(loadPercent), 0.7);

        double wear = baseRate * loadF * tempFactor * oilFactor;

        log("CylinderWear: load=" + std::to_string(loadPercent) +
            " t=" + std::to_string(oilTemp) +
            " oilQ=" + std::to_string(oilQuality) +
            " -> " + std::to_string(wear) + " мм/ч");

        return wear;
    }

    double EngineModel::calculateOilDegradation(double oilTemp, double loadPercent) const
    {
        constexpr double BASE_DEGRADATION = 100.0 / 250.0;

        double tempFactor = temperatureFactor(oilTemp);
        double loadF = std::pow(loadFactor(loadPercent), 0.6);

        double degradation = BASE_DEGRADATION * tempFactor * loadF;

        log("OilDegradation: t=" + std::to_string(oilTemp) +
            " load=" + std::to_string(loadPercent) +
            " -> " + std::to_string(degradation) + " %/ч");

        return degradation;
    }

    double EngineModel::calculateCurrentPower(double pistonWear, double cylinderWear, double fuelEfficiency) const
    {
        double ringFactor = 1.0 - (pistonWear / m_config.maxWear) * 0.15;
        double cylFactor = 1.0 - (cylinderWear / m_config.maxWear) * 0.10;
        double fuelFactor = fuelEfficiency / 100.0;

        double power = m_config.nominalPower_kW * ringFactor * cylFactor * fuelFactor;

        log("CurrentPower: ringWear=" + std::to_string(pistonWear) +
            " cylWear=" + std::to_string(cylinderWear) +
            " -> " + std::to_string(power) + " кВт");

        return std::max(0.0, power);
    }

    double EngineModel::calculateFuelConsumption(double loadPercent, double currentPower_kW) const
    {
        double actualPower = currentPower_kW * loadFactor(loadPercent);
        return m_config.fuelConsumptionBase * actualPower;
    }

    double EngineModel::calculateHeatEmission(double loadPercent, double currentPower_kW) const
    {
        double actualPower = currentPower_kW * loadFactor(loadPercent);
        return actualPower * 0.6;
    }

    EnginePrediction EngineModel::predict(const EngineState& state, double loadPercent, double fuelEfficiency) const
    {
        EnginePrediction result{};

        double ringWearRate = calculatePistonWear(loadPercent, state.oilTemperature, state.oilQuality);
        double cylWearRate = calculateCylinderWear(loadPercent, state.oilTemperature, state.oilQuality);
        double oilDegRate = calculateOilDegradation(state.oilTemperature, loadPercent);

        double ringRemaining = m_config.maxWear - state.pistonRingWear;
        double cylRemaining = m_config.maxWear - state.cylinderWear;

        result.pistonLife_hours = (ringWearRate > 0.0) ? ringRemaining / ringWearRate : 1e9;
        result.cylinderLife_hours = (cylWearRate > 0.0) ? cylRemaining / cylWearRate : 1e9;
        result.oilLife_hours = (oilDegRate > 0.0) ? (state.oilQuality - 20.0) / oilDegRate : 1e9;

        result.pistonLife_hours = std::max(0.0, result.pistonLife_hours);
        result.cylinderLife_hours = std::max(0.0, result.cylinderLife_hours);
        result.oilLife_hours = std::max(0.0, result.oilLife_hours);

        result.currentPower_kW = calculateCurrentPower(state.pistonRingWear, state.cylinderWear, fuelEfficiency);
        result.powerPercent = (result.currentPower_kW / m_config.nominalPower_kW) * 100.0;

        result.fuelConsumption = calculateFuelConsumption(loadPercent, result.currentPower_kW);
        result.heatEmission_kW = calculateHeatEmission(loadPercent, result.currentPower_kW);


        double wearRatio = std::max(state.pistonRingWear / m_config.maxWear, state.cylinderWear / m_config.maxWear);
        result.isCritical = (wearRatio >= 0.8) || (state.oilQuality <= 20.0);

        std::ostringstream rec;
        if (result.isCritical)
            rec << "КРИТИЧЕСКОЕ СОСТОЯНИЕ! ";

        if (result.oilLife_hours < 50.0)
            rec << "Замена масла через " << static_cast<int>(result.oilLife_hours) << " ч. ";

        if (result.pistonLife_hours < 500.0)
            rec << "Замена колец через " << static_cast<int>(result.pistonLife_hours) << " ч. ";

        if (result.cylinderLife_hours < 500.0)
            rec << "Ремонт цилиндров через " << static_cast<int>(result.cylinderLife_hours) << " ч. ";

        if (rec.str().empty())
            rec << "Состояние в норме.";

        result.recommendation = rec.str();

        return result;
    }
}