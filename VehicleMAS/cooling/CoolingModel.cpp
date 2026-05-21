#include "CoolingModel.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace mas::model
{
	CoolingModel::CoolingModel(const CoolingConfig& config, bool verbose): m_config(config), m_verbose(verbose)
	{
	}

	void CoolingModel::log(const std::string& message) const
	{
		if (m_verbose)
			std::cout << "  [COOLING_VERBOSE] " << message << std::endl;
	}

	double CoolingModel::calculateCoolantTemp(double loadPercent, double externalTemp, double radiatorClog, double pumpWear) const
	{
		double loadF = loadPercent / 100.0;
		double targetTemp = m_config.nominalCoolantTemp + (loadF - 0.5) * 15.0;
		double extInfluence = (externalTemp - 20.0) * 0.3;
		double radFactor = 1.0 + (radiatorClog / m_config.maxRadiatorClog) * 0.8;
		double pumpFactor = 1.0 + (pumpWear / m_config.maxPumpWear) * 0.5;

		double temp = targetTemp + extInfluence;

		temp += (radFactor - 1.0) * 25.0;
		temp += (pumpFactor - 1.0) * 15.0;

		temp = std::clamp(temp, externalTemp, m_config.maxCoolantTemp + 10.0);

		log("CoolantTemp: load=" + std::to_string(loadPercent) +
			" extTemp=" + std::to_string(externalTemp) +
			" target=" + std::to_string(targetTemp) +
			" extInfluence=" + std::to_string(extInfluence) +
			" radClog=" + std::to_string(radiatorClog) +
			" pumpWear=" + std::to_string(pumpWear) +
			" radF=" + std::to_string(radFactor) +
			" pumpF=" + std::to_string(pumpFactor) +
			" -> " + std::to_string(temp) + " °C");

		return temp;
	}

	double CoolingModel::calculateCoolantDegradation(double coolantTemp) const
	{
		constexpr double BASE_DEGRADATION = 100.0 / 2000.0;

		double tempFactor = 1.0;
		if (coolantTemp > m_config.nominalCoolantTemp)
		{
			double overheat = (coolantTemp - m_config.nominalCoolantTemp) / (m_config.maxCoolantTemp - m_config.nominalCoolantTemp);
			overheat = std::clamp(overheat, 0.0, 1.0);
			tempFactor = 1.0 + 3.0 * std::pow(overheat, 2.0);
		}

		double degradation = BASE_DEGRADATION * tempFactor;

		log("CoolantDegradation: temp=" + std::to_string(coolantTemp) +
			" tempFactor=" + std::to_string(tempFactor) +
			" -> " + std::to_string(degradation) + " %/ч");

		return degradation;
	}

	double CoolingModel::calculateRadiatorClogRate(double externalTemp) const
	{
		double baseRate = 100.0 / 5000.0;

		double tempFactor = 1.0;
		if (externalTemp > 25.0)
			tempFactor = 1.0 + (externalTemp - 25.0) * 0.04;

		double rate = baseRate * tempFactor;

		log("RadiatorClog: extTemp=" + std::to_string(externalTemp) +
			" tempFactor=" + std::to_string(tempFactor) +
			" -> " + std::to_string(rate) + " %/ч");

		return rate;
	}

	double CoolingModel::calculatePumpWear(double coolantTemp) const
	{
		double baseRate = m_config.maxPumpWear / 6000.0;

		double tempFactor = 1.0;
		if (coolantTemp > m_config.nominalCoolantTemp)
		{
			double overheat = (coolantTemp - m_config.nominalCoolantTemp) / (m_config.maxCoolantTemp - m_config.nominalCoolantTemp);
			overheat = std::clamp(overheat, 0.0, 1.0);
			tempFactor = 1.0 + 2.0 * overheat;
		}

		double wear = baseRate * tempFactor;

		log("PumpWear: temp=" + std::to_string(coolantTemp) +
			" tempFactor=" + std::to_string(tempFactor) +
			" -> " + std::to_string(wear) + " %/ч");

		return wear;
	}

	CoolingPrediction CoolingModel::predict(const CoolingState& state, double loadPercent, double externalTemp) const
	{
		CoolingPrediction result{};

		result.predictedCoolantTemp = calculateCoolantTemp(loadPercent, externalTemp, state.radiatorClog, state.pumpWear);

		double coolantDegRate = calculateCoolantDegradation(state.coolantTemp);
		double clogRate = calculateRadiatorClogRate(externalTemp);
		double pumpRate = calculatePumpWear(state.coolantTemp);

		double coolantRemaining = state.coolantQuality - 20.0;
		double radiatorRemaining = m_config.maxRadiatorClog - state.radiatorClog;
		double pumpRemaining = m_config.maxPumpWear - state.pumpWear;

		result.coolantLife_hours = (coolantDegRate > 0.0) ? coolantRemaining / coolantDegRate : 1e9;
		result.radiatorClean_hours = (clogRate > 0.0) ? radiatorRemaining / clogRate : 1e9;
		result.pumpLife_hours = (pumpRate > 0.0) ? pumpRemaining / pumpRate : 1e9;

		result.coolantLife_hours = std::max(0.0, result.coolantLife_hours);
		result.radiatorClean_hours = std::max(0.0, result.radiatorClean_hours);
		result.pumpLife_hours = std::max(0.0, result.pumpLife_hours);

		result.isOverheat = result.predictedCoolantTemp >= m_config.maxCoolantTemp * 0.9;

		double radRatio = state.radiatorClog / m_config.maxRadiatorClog;
		double pumpRatio = state.pumpWear / m_config.maxPumpWear;
		result.isCritical = result.isOverheat || (radRatio >= 0.8) || (pumpRatio >= 0.8) || (state.coolantQuality <= 20.0);

		std::ostringstream rec;
		if (result.isOverheat)
			rec << "ПЕРЕГРЕВ ОЖ! Проверьте радиатор и помпу. ";

		if (result.isCritical && !result.isOverheat)
			rec << "КРИТИЧЕСКОЕ СОСТОЯНИЕ! ";

		if (result.coolantLife_hours < 200.0)
			rec << "Замена ОЖ через " << static_cast<int>(result.coolantLife_hours) << " ч. ";

		if (result.radiatorClean_hours < 300.0)
			rec << "Чистка радиатора через " << static_cast<int>(result.radiatorClean_hours) << " ч. ";

		if (result.pumpLife_hours < 400.0)
			rec << "Замена помпы через " << static_cast<int>(result.pumpLife_hours) << " ч. ";

		if (rec.str().empty())
		{
			double minLife = std::min({ result.coolantLife_hours, result.radiatorClean_hours, result.pumpLife_hours });
			rec << "Система охлаждения в норме. Ближайшее ТО через " << static_cast<int>(minLife) << " ч.";
		}
		result.recommendation = rec.str();

		log("PREDICT: coolantTemp=" + std::to_string(result.predictedCoolantTemp) +
			" coolantLife=" + std::to_string(result.coolantLife_hours) +
			" radClean=" + std::to_string(result.radiatorClean_hours) +
			" pumpLife=" + std::to_string(result.pumpLife_hours) +
			" overheat=" + std::string(result.isOverheat ? "YES" : "NO") +
			" critical=" + std::string(result.isCritical ? "YES" : "NO"));

		return result;
	}
}