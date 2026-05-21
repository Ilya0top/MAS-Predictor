#include "BrakeModel.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace mas::model
{
    BrakeModel::BrakeModel(const BrakeConfig& config, bool verbose) : m_config(config), m_verbose(verbose)
    {
    }

    void BrakeModel::log(const std::string& message) const
    {
        if (m_verbose)
            std::cout << "  [BRAKE_VERBOSE] " << message << std::endl;
    }

    double BrakeModel::calculatePadWear(int brakesPerHour, double brakeTemp, double loadPercent) const
    {
        double baseRate = m_config.maxPadWear / m_config.maxPadLife;
        double brakeFactor = brakesPerHour / 5.0;
        double tempFactor = 1.0;

        if (brakeTemp > m_config.brakeTempBase)
        {
            double overheat = (brakeTemp - m_config.brakeTempBase) / (m_config.maxBrakeTemp - m_config.brakeTempBase);
            overheat = std::clamp(overheat, 0.0, 1.0);
            tempFactor = 1.0 + 3.0 * std::pow(overheat, 1.5);
        }

        double loadF = std::pow(loadPercent / 100.0, 0.8);

        double wear = baseRate * brakeFactor * tempFactor * loadF;

        log("PadWear: brakes=" + std::to_string(brakesPerHour) +
            " temp=" + std::to_string(brakeTemp) +
            " load=" + std::to_string(loadPercent) +
            " baseRate=" + std::to_string(baseRate) +
            " brakeFactor=" + std::to_string(brakeFactor) +
            " tempFactor=" + std::to_string(tempFactor) +
            " loadF=" + std::to_string(loadF) +
            " -> " + std::to_string(wear) + " ìì/÷");

        return wear;
    }

    double BrakeModel::calculateDiscWear(int brakesPerHour, double brakeTemp, double loadPercent) const
    {
        double baseRate = m_config.maxDiscWear / m_config.maxDiscLife;
        double brakeFactor = brakesPerHour / 5.0;
        double tempFactor = 1.0;

        if (brakeTemp > m_config.brakeTempBase)
        {
            double overheat = (brakeTemp - m_config.brakeTempBase) / (m_config.maxBrakeTemp - m_config.brakeTempBase);
            overheat = std::clamp(overheat, 0.0, 1.0);
            tempFactor = 1.0 + 5.0 * std::pow(overheat, 2.0);
        }

        double loadF = std::pow(loadPercent / 100.0, 0.9);

        double wear = baseRate * brakeFactor * tempFactor * loadF;

        log("DiscWear: brakes=" + std::to_string(brakesPerHour) +
            " temp=" + std::to_string(brakeTemp) +
            " load=" + std::to_string(loadPercent) +
            " baseRate=" + std::to_string(baseRate) +
            " brakeFactor=" + std::to_string(brakeFactor) +
            " tempFactor=" + std::to_string(tempFactor) +
            " loadF=" + std::to_string(loadF) +
            " -> " + std::to_string(wear) + " ìì/÷");

        return wear;
    }

    double BrakeModel::calculateBrakeTemp(double loadPercent, int brakesPerHour, double currentTemp) const
    {
        double heatGain = (loadPercent / 100.0) * (brakesPerHour / 5.0) * 25.0;
        double cooling = (currentTemp - 20.0) * 0.15;
        double newTemp = currentTemp + heatGain - cooling;

        newTemp = std::clamp(newTemp, 20.0, m_config.maxBrakeTemp);

        log("BrakeTemp: load=" + std::to_string(loadPercent) +
            " brakes=" + std::to_string(brakesPerHour) +
            " current=" + std::to_string(currentTemp) +
            " heat=" + std::to_string(heatGain) +
            " cool=" + std::to_string(cooling) +
            " -> " + std::to_string(newTemp) + " °C");

        return newTemp;
    }

    BrakePrediction BrakeModel::predict(const BrakeState& state, double loadPercent) const
    {
        BrakePrediction result{};

        double padRate = calculatePadWear(state.brakesPerHour, state.brakeTemp, loadPercent);
        double discRate = calculateDiscWear(state.brakesPerHour, state.brakeTemp, loadPercent);

        double padRemaining = m_config.maxPadWear - state.padWear;
        double discRemaining = m_config.maxDiscWear - state.discWear;

        result.padLife_hours = (padRate > 0.0) ? padRemaining / padRate : 1e9;
        result.discLife_hours = (discRate > 0.0) ? discRemaining / discRate : 1e9;

        result.padLife_hours = std::max(0.0, result.padLife_hours);
        result.discLife_hours = std::max(0.0, result.discLife_hours);

        result.isOverheat = state.brakeTemp >= m_config.maxBrakeTemp * 0.85;

        double padRatio = state.padWear / m_config.maxPadWear;
        double discRatio = state.discWear / m_config.maxDiscWear;
        result.isCritical = (padRatio >= 0.85) || (discRatio >= 0.85) || result.isOverheat;

        std::ostringstream rec;
        if (result.isOverheat)
            rec << "ÏÅÐÅÃÐÅÂ ÒÎÐÌÎÇÎÂ! Ñíèçüòå ÷àñòîòó òîðìîæåíèé. ";

        if (result.isCritical && !result.isOverheat)
            rec << "ÊÐÈÒÈ×ÅÑÊÈÉ ÈÇÍÎÑ! ";

        if (result.padLife_hours < 200.0)
            rec << "Çàìåíà êîëîäîê ÷åðåç " << static_cast<int>(result.padLife_hours) << " ÷. ";

        if (result.discLife_hours < 500.0)
            rec << "Çàìåíà äèñêîâ ÷åðåç " << static_cast<int>(result.discLife_hours) << " ÷. ";

        if (rec.str().empty())
        {
            double minLife = std::min(result.padLife_hours, result.discLife_hours);
            rec << "Òîðìîçà â íîðìå. Áëèæàéøàÿ çàìåíà ÷åðåç " << static_cast<int>(minLife) << " ÷.";
        }
        result.recommendation = rec.str();

        log("PREDICT: padLife=" + std::to_string(result.padLife_hours) +
            " discLife=" + std::to_string(result.discLife_hours) +
            " overheat=" + std::string(result.isOverheat ? "YES" : "NO") +
            " critical=" + std::string(result.isCritical ? "YES" : "NO"));

        return result;
    }
}